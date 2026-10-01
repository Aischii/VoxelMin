#include "world/ChunkMesher.hpp"
#include "core/Config.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <vector>

namespace vox {
namespace {

constexpr float TILE_UV = 1.0f / static_cast<float>(config::ATLAS_TILES);

inline TextureTile getFaceTile(BlockId id, const BlockDef& def, const glm::ivec3& norm) {
    if (id == BlockId::WoodX) {
        if (norm.x != 0) return TextureTile::WoodTop;
        return TextureTile::WoodSide;
    }
    if (id == BlockId::WoodZ) {
        if (norm.z != 0) return TextureTile::WoodTop;
        return TextureTile::WoodSide;
    }
    if (norm.y > 0) return def.top;
    if (norm.y < 0) return def.bottom;
    return def.side;
}

struct FaceSpec {
    glm::ivec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

const FaceSpec FACE_SPECS[6] = {
    {{ 1,  0,  0}, { 0, 0, -1}, { 0, 1,  0}}, // +X
    {{-1,  0,  0}, { 0, 0,  1}, { 0, 1,  0}}, // -X
    {{ 0,  1,  0}, { 1, 0,  0}, { 0, 0, -1}}, // +Y
    {{ 0, -1,  0}, { 1, 0,  0}, { 0, 0,  1}}, // -Y
    {{ 0,  0,  1}, { 1, 0,  0}, { 0, 1,  0}}, // +Z
    {{ 0,  0, -1}, {-1, 0,  0}, { 0, 1,  0}}, // -Z
};

inline uint8_t computeVertexAO(const World& world, int fx, int fy, int fz,
                               const glm::ivec3& tangDir,
                               const glm::ivec3& bitangDir) {
    const bool sideA = isOpaque(world.getBlock(fx + tangDir.x, fy + tangDir.y, fz + tangDir.z));
    const bool sideB = isOpaque(world.getBlock(fx + bitangDir.x, fy + bitangDir.y, fz + bitangDir.z));
    const bool corner = isOpaque(world.getBlock(fx + tangDir.x + bitangDir.x,
                                               fy + tangDir.y + bitangDir.y,
                                               fz + tangDir.z + bitangDir.z));

    if (sideA && sideB) return 0;
    return static_cast<uint8_t>(3 - (sideA + sideB + corner));
}

inline float aoToFloat(uint8_t ao) {
    switch (ao) {
        case 0: return 0.60f;
        case 1: return 0.75f;
        case 2: return 0.88f;
        default: return 1.00f;
    }
}

struct FaceMask {
    BlockId block = BlockId::Air;
    TextureTile tile = TextureTile::GrassTop;
    bool isTransparent = false;
    uint8_t ao0 = 3;
    uint8_t ao1 = 3;
    uint8_t ao2 = 3;
    uint8_t ao3 = 3;
    uint8_t light = 15;

    bool operator==(const FaceMask& o) const {
        return block == o.block && tile == o.tile && isTransparent == o.isTransparent &&
               ao0 == o.ao0 && ao1 == o.ao1 && ao2 == o.ao2 && ao3 == o.ao3 &&
               light == o.light;
    }
    bool operator!=(const FaceMask& o) const { return !(*this == o); }
    bool empty() const { return block == BlockId::Air; }
};

void emitQuad(std::vector<Vertex>& outVertices,
              std::vector<uint32_t>& outIndices,
              const glm::vec3& p0,
              const glm::vec3& p1,
              const glm::vec3& p2,
              const glm::vec3& p3,
              const glm::vec3& normal,
              float w, float h,
              TextureTile tile,
              const float ao[4],
              float light) {
    const int tileIdx = static_cast<int>(tile);
    const int tileCol = tileIdx % config::ATLAS_TILES;
    const int tileRow = tileIdx / config::ATLAS_TILES;
    const float u0 = static_cast<float>(tileCol) * TILE_UV;
    const float v0 = static_cast<float>(tileRow) * TILE_UV;
    const glm::vec2 tileMin(u0, v0);
    const glm::vec2 tileSize(TILE_UV, TILE_UV);

    const uint32_t base = static_cast<uint32_t>(outVertices.size());
    outVertices.push_back({ p0, normal, {0.0f, 0.0f}, tileMin, tileSize, ao[0], light });
    outVertices.push_back({ p1, normal, {w,    0.0f}, tileMin, tileSize, ao[1], light });
    outVertices.push_back({ p2, normal, {w,    h},    tileMin, tileSize, ao[2], light });
    outVertices.push_back({ p3, normal, {0.0f, h},    tileMin, tileSize, ao[3], light });

    if (ao[0] + ao[2] < ao[1] + ao[3]) {
        outIndices.push_back(base + 1);
        outIndices.push_back(base + 2);
        outIndices.push_back(base + 3);
        outIndices.push_back(base + 1);
        outIndices.push_back(base + 3);
        outIndices.push_back(base + 0);
    } else {
        outIndices.push_back(base + 0);
        outIndices.push_back(base + 1);
        outIndices.push_back(base + 2);
        outIndices.push_back(base + 0);
        outIndices.push_back(base + 2);
        outIndices.push_back(base + 3);
    }
}

void emitTorch3D(std::vector<Vertex>& outVertices,
                 std::vector<uint32_t>& outIndices,
                 float wx, float wy, float wz,
                 BlockId torchType,
                 float light) {
    const TextureTile tile = TextureTile::Planks;
    const float ao[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float halfW = 0.0625f;

    if (torchType == BlockId::TorchWallWest) {
        // Mounted on West wall (x = wx). Base is at x = wx + 0.06, y = wy + 0.20.
        // Slanted toward +X: Top is at x = wx + 0.32, y = wy + 0.72.
        const float zMid = wz + 0.5f;
        const glm::vec3 baseCenter(wx + 0.06f, wy + 0.20f, zMid);
        const glm::vec3 topCenter(wx + 0.32f, wy + 0.72f, zMid);

        const glm::vec3 b0 = baseCenter + glm::vec3(-0.04f, -0.04f, -halfW);
        const glm::vec3 b1 = baseCenter + glm::vec3( 0.04f, -0.04f,  halfW);
        const glm::vec3 b2 = baseCenter + glm::vec3( 0.04f,  0.04f,  halfW);
        const glm::vec3 b3 = baseCenter + glm::vec3(-0.04f,  0.04f, -halfW);

        const glm::vec3 t0 = topCenter + glm::vec3(-halfW, 0.0f, -halfW);
        const glm::vec3 t1 = topCenter + glm::vec3( halfW, 0.0f, -halfW);
        const glm::vec3 t2 = topCenter + glm::vec3( halfW, 0.0f,  halfW);
        const glm::vec3 t3 = topCenter + glm::vec3(-halfW, 0.0f,  halfW);

        emitQuad(outVertices, outIndices, t3, t2, t1, t0, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, b1, b2, b3, {-0.89f, -0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b1, t2, t3, b0, {0.0f, 0.0f, 1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t0, t1, b2, {0.0f, 0.0f, -1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b2, t1, t2, b1, {0.89f, -0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, t3, t0, b3, {-0.89f, 0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
    } else if (torchType == BlockId::TorchWallEast) {
        // Mounted on East wall (x = wx + 1). Base is at x = wx + 0.94, y = wy + 0.20.
        // Slanted toward -X: Top is at x = wx + 0.68, y = wy + 0.72.
        const float zMid = wz + 0.5f;
        const glm::vec3 baseCenter(wx + 0.94f, wy + 0.20f, zMid);
        const glm::vec3 topCenter(wx + 0.68f, wy + 0.72f, zMid);

        const glm::vec3 b0 = baseCenter + glm::vec3( 0.04f, -0.04f, -halfW);
        const glm::vec3 b1 = baseCenter + glm::vec3(-0.04f, -0.04f,  halfW);
        const glm::vec3 b2 = baseCenter + glm::vec3(-0.04f,  0.04f,  halfW);
        const glm::vec3 b3 = baseCenter + glm::vec3( 0.04f,  0.04f, -halfW);

        const glm::vec3 t0 = topCenter + glm::vec3(-halfW, 0.0f, -halfW);
        const glm::vec3 t1 = topCenter + glm::vec3( halfW, 0.0f, -halfW);
        const glm::vec3 t2 = topCenter + glm::vec3( halfW, 0.0f,  halfW);
        const glm::vec3 t3 = topCenter + glm::vec3(-halfW, 0.0f,  halfW);

        emitQuad(outVertices, outIndices, t3, t2, t1, t0, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, b1, b2, b3, {0.89f, -0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b1, t2, t3, b0, {0.0f, 0.0f, 1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t0, t1, b2, {0.0f, 0.0f, -1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b2, t3, t0, b3, {-0.89f, -0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, t1, t2, b1, {0.89f, 0.44f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
    } else if (torchType == BlockId::TorchWallNorth) {
        // Mounted on North wall (z = wz). Base is at z = wz + 0.06, y = wy + 0.20.
        // Slanted toward +Z: Top is at z = wz + 0.32, y = wy + 0.72.
        const float xMid = wx + 0.5f;
        const glm::vec3 baseCenter(xMid, wy + 0.20f, wz + 0.06f);
        const glm::vec3 topCenter(xMid, wy + 0.72f, wz + 0.32f);

        const glm::vec3 b0 = baseCenter + glm::vec3(-halfW, -0.04f, -0.04f);
        const glm::vec3 b1 = baseCenter + glm::vec3( halfW, -0.04f,  0.04f);
        const glm::vec3 b2 = baseCenter + glm::vec3( halfW,  0.04f,  0.04f);
        const glm::vec3 b3 = baseCenter + glm::vec3(-halfW,  0.04f, -0.04f);

        const glm::vec3 t0 = topCenter + glm::vec3(-halfW, 0.0f, -halfW);
        const glm::vec3 t1 = topCenter + glm::vec3( halfW, 0.0f, -halfW);
        const glm::vec3 t2 = topCenter + glm::vec3( halfW, 0.0f,  halfW);
        const glm::vec3 t3 = topCenter + glm::vec3(-halfW, 0.0f,  halfW);

        emitQuad(outVertices, outIndices, t3, t2, t1, t0, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, b1, b2, b3, {0.0f, -0.44f, -0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, t3, t2, b1, {0.0f, 0.44f, 0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t1, t0, b2, {0.0f, -0.44f, -0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b1, t2, t1, b2, {1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t0, t3, b0, {-1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
    } else if (torchType == BlockId::TorchWallSouth) {
        // Mounted on South wall (z = wz + 1). Base is at z = wz + 0.94, y = wy + 0.20.
        // Slanted toward -Z: Top is at z = wz + 0.68, y = wy + 0.72.
        const float xMid = wx + 0.5f;
        const glm::vec3 baseCenter(xMid, wy + 0.20f, wz + 0.94f);
        const glm::vec3 topCenter(xMid, wy + 0.72f, wz + 0.68f);

        const glm::vec3 b0 = baseCenter + glm::vec3(-halfW, -0.04f,  0.04f);
        const glm::vec3 b1 = baseCenter + glm::vec3( halfW, -0.04f, -0.04f);
        const glm::vec3 b2 = baseCenter + glm::vec3( halfW,  0.04f, -0.04f);
        const glm::vec3 b3 = baseCenter + glm::vec3(-halfW,  0.04f,  0.04f);

        const glm::vec3 t0 = topCenter + glm::vec3(-halfW, 0.0f, -halfW);
        const glm::vec3 t1 = topCenter + glm::vec3( halfW, 0.0f, -halfW);
        const glm::vec3 t2 = topCenter + glm::vec3( halfW, 0.0f,  halfW);
        const glm::vec3 t3 = topCenter + glm::vec3(-halfW, 0.0f,  halfW);

        emitQuad(outVertices, outIndices, t3, t2, t1, t0, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, b1, b2, b3, {0.0f, -0.44f, 0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b0, t3, t2, b1, {0.0f, -0.44f, -0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t1, t0, b2, {0.0f, 0.44f, 0.89f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b1, t0, t1, b2, {1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices, b3, t2, t3, b0, {-1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
    } else {
        // Floor Torch (standing upright)
        const float x0 = wx + 0.4375f;
        const float x1 = wx + 0.5625f;
        const float z0 = wz + 0.4375f;
        const float z1 = wz + 0.5625f;
        const float y0 = wy;
        const float y1 = wy + 0.625f;

        emitQuad(outVertices, outIndices,
                 {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0},
                 {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices,
                 {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1},
                 {0.0f, 0.0f, 1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices,
                 {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0},
                 {0.0f, 0.0f, -1.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices,
                 {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1},
                 {1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices,
                 {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0},
                 {-1.0f, 0.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
        emitQuad(outVertices, outIndices,
                 {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1},
                 {0.0f, -1.0f, 0.0f}, 1.0f, 1.0f, tile, ao, light);
    }
}

void emitCrossModel(std::vector<Vertex>& outVertices,
                    std::vector<uint32_t>& outIndices,
                    float wx, float wy, float wz,
                    TextureTile tile,
                    float light) {
    const float x0 = wx;
    const float x1 = wx + 1.0f;
    const float z0 = wz;
    const float z1 = wz + 1.0f;
    const float y0 = wy;
    const float y1 = wy + 1.0f;

    const float ao[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // Diagonal 1 (from (x0, z0) to (x1, z1))
    emitQuad(outVertices, outIndices,
             {x0, y0, z0}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z0},
             {0.7071f, 0.0f, -0.7071f}, 1.0f, 1.0f, tile, ao, light);

    // Diagonal 2 (from (x0, z1) to (x1, z0))
    emitQuad(outVertices, outIndices,
             {x0, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z1},
             {0.7071f, 0.0f, 0.7071f}, 1.0f, 1.0f, tile, ao, light);
}

inline bool isSpecialModel(BlockId b) {
    return isTorch(b) || b == BlockId::TallGrass;
}

} // namespace

void buildChunkGeometry(const World& world,
                        const Chunk& chunk,
                        std::vector<Vertex>& outOpaqueVertices,
                        std::vector<uint32_t>& outOpaqueIndices,
                        std::vector<Vertex>& outTransVertices,
                        std::vector<uint32_t>& outTransIndices) {
    outOpaqueVertices.clear();
    outOpaqueIndices.clear();
    outTransVertices.clear();
    outTransIndices.clear();

    const int originX = chunk.originX();
    const int originZ = chunk.originZ();

    // 1. Greedy Meshing along X (+X and -X faces)
    // Slice: X in [0, W-1], 2D grid: Z in [0, D-1] (U=D), Y in [0, H-1] (V=H)
    for (int faceIdx = 0; faceIdx < 2; ++faceIdx) {
        const FaceSpec& spec = FACE_SPECS[faceIdx];
        const glm::ivec3& norm = spec.normal;
        const glm::vec3 normF(norm);
        const glm::ivec3 tangI = glm::ivec3(spec.tangent);
        const glm::ivec3 bitangI = glm::ivec3(spec.bitangent);

        for (int x = 0; x < Chunk::W; ++x) {
            std::vector<FaceMask> mask(static_cast<size_t>(Chunk::D) * Chunk::H);

            for (int y = 0; y < Chunk::H; ++y) {
                for (int z = 0; z < Chunk::D; ++z) {
                    const BlockId block = chunk.get(x, y, z);
                    if (isAir(block) || isSpecialModel(block)) continue;

                    const int wx = originX + x;
                    const int wz = originZ + z;
                    const int fx = wx + norm.x;
                    const int fy = y + norm.y;
                    const int fz = wz + norm.z;
                    const BlockId nBlock = world.getBlock(fx, fy, fz);

                    const bool curTrans = isTransparent(block);
                    if (curTrans) {
                        if (nBlock == block || isOpaque(nBlock)) continue;
                    } else {
                        if (isOpaque(nBlock)) continue;
                    }

                    const BlockDef& def = blockDef(block);
                    const TextureTile tile = getFaceTile(block, def, norm);

                    const uint8_t ao0 = computeVertexAO(world, fx, fy, fz, -tangI, -bitangI);
                    const uint8_t ao1 = computeVertexAO(world, fx, fy, fz,  tangI, -bitangI);
                    const uint8_t ao2 = computeVertexAO(world, fx, fy, fz,  tangI,  bitangI);
                    const uint8_t ao3 = computeVertexAO(world, fx, fy, fz, -tangI,  bitangI);

                    const float lightF = world.skyLight(fx, fy, fz);
                    const uint8_t lightU = static_cast<uint8_t>(std::clamp(static_cast<int>(lightF * 15.0f + 0.5f), 0, 15));

                    mask[static_cast<size_t>(y) * Chunk::D + z] = { block, tile, curTrans, ao0, ao1, ao2, ao3, lightU };
                }
            }

            // Greedy merge on mask (U = Chunk::D, V = Chunk::H)
            for (int y = 0; y < Chunk::H; ++y) {
                for (int z = 0; z < Chunk::D;) {
                    const FaceMask current = mask[static_cast<size_t>(y) * Chunk::D + z];
                    if (current.empty()) {
                        z++;
                        continue;
                    }

                    // Compute width along Z
                    int w = 1;
                    while (z + w < Chunk::D && mask[static_cast<size_t>(y) * Chunk::D + (z + w)] == current) {
                        w++;
                    }

                    // Compute height along Y
                    int h = 1;
                    bool canExtend = true;
                    while (y + h < Chunk::H && canExtend) {
                        for (int k = 0; k < w; ++k) {
                            if (mask[static_cast<size_t>(y + h) * Chunk::D + (z + k)] != current) {
                                canExtend = false;
                                break;
                            }
                        }
                        if (canExtend) h++;
                    }

                    // Clear visited rectangle
                    for (int dy = 0; dy < h; ++dy) {
                        for (int dz = 0; dz < w; ++dz) {
                            mask[static_cast<size_t>(y + dy) * Chunk::D + (z + dz)] = FaceMask{};
                        }
                    }

                    // Emit merged quad
                    const float fx = static_cast<float>(originX + x);
                    const float fy = static_cast<float>(y);
                    const float fz = static_cast<float>(originZ + z);
                    const float fw = static_cast<float>(w);
                    const float fh = static_cast<float>(h);

                    glm::vec3 p0, p1, p2, p3;
                    if (faceIdx == 0) { // +X
                        const glm::vec3 base(fx + 1.0f, fy, fz + fw);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;
                    } else { // -X
                        const glm::vec3 base(fx, fy, fz);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;
                    }

                    const float quadAO[4] = {
                        aoToFloat(current.ao0),
                        aoToFloat(current.ao1),
                        aoToFloat(current.ao2),
                        aoToFloat(current.ao3)
                    };
                    const float quadLight = static_cast<float>(current.light) / 15.0f;

                    if (current.isTransparent) {
                        emitQuad(outTransVertices, outTransIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    } else {
                        emitQuad(outOpaqueVertices, outOpaqueIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    }

                    z += w;
                }
            }
        }
    }

    // 2. Greedy Meshing along Y (+Y and -Y faces)
    // Slice: Y in [0, H-1], 2D grid: X in [0, W-1] (U=W), Z in [0, D-1] (V=D)
    for (int faceIdx = 2; faceIdx < 4; ++faceIdx) {
        const FaceSpec& spec = FACE_SPECS[faceIdx];
        const glm::ivec3& norm = spec.normal;
        const glm::vec3 normF(norm);
        const glm::ivec3 tangI = glm::ivec3(spec.tangent);
        const glm::ivec3 bitangI = glm::ivec3(spec.bitangent);

        for (int y = 0; y < Chunk::H; ++y) {
            std::vector<FaceMask> mask(static_cast<size_t>(Chunk::W) * Chunk::D);

            for (int z = 0; z < Chunk::D; ++z) {
                for (int x = 0; x < Chunk::W; ++x) {
                    const BlockId block = chunk.get(x, y, z);
                    if (isAir(block) || isSpecialModel(block)) continue;

                    const int wx = originX + x;
                    const int wz = originZ + z;
                    const int fx = wx + norm.x;
                    const int fy = y + norm.y;
                    const int fz = wz + norm.z;
                    const BlockId nBlock = world.getBlock(fx, fy, fz);

                    const bool curTrans = isTransparent(block);
                    if (curTrans) {
                        if (nBlock == block || isOpaque(nBlock)) continue;
                    } else {
                        if (isOpaque(nBlock)) continue;
                    }

                    const BlockDef& def = blockDef(block);
                    const TextureTile tile = getFaceTile(block, def, norm);

                    const uint8_t ao0 = computeVertexAO(world, fx, fy, fz, -tangI, -bitangI);
                    const uint8_t ao1 = computeVertexAO(world, fx, fy, fz,  tangI, -bitangI);
                    const uint8_t ao2 = computeVertexAO(world, fx, fy, fz,  tangI,  bitangI);
                    const uint8_t ao3 = computeVertexAO(world, fx, fy, fz, -tangI,  bitangI);

                    const float lightF = world.skyLight(fx, fy, fz);
                    const uint8_t lightU = static_cast<uint8_t>(std::clamp(static_cast<int>(lightF * 15.0f + 0.5f), 0, 15));

                    mask[static_cast<size_t>(z) * Chunk::W + x] = { block, tile, curTrans, ao0, ao1, ao2, ao3, lightU };
                }
            }

            // Greedy merge on mask (U = Chunk::W, V = Chunk::D)
            for (int z = 0; z < Chunk::D; ++z) {
                for (int x = 0; x < Chunk::W;) {
                    const FaceMask current = mask[static_cast<size_t>(z) * Chunk::W + x];
                    if (current.empty()) {
                        x++;
                        continue;
                    }

                    // Compute width along X
                    int w = 1;
                    while (x + w < Chunk::W && mask[static_cast<size_t>(z) * Chunk::W + (x + w)] == current) {
                        w++;
                    }

                    // Compute height along Z
                    int h = 1;
                    bool canExtend = true;
                    while (z + h < Chunk::D && canExtend) {
                        for (int k = 0; k < w; ++k) {
                            if (mask[static_cast<size_t>(z + h) * Chunk::W + (x + k)] != current) {
                                canExtend = false;
                                break;
                            }
                        }
                        if (canExtend) h++;
                    }

                    // Clear visited rectangle
                    for (int dz = 0; dz < h; ++dz) {
                        for (int dx = 0; dx < w; ++dx) {
                            mask[static_cast<size_t>(z + dz) * Chunk::W + (x + dx)] = FaceMask{};
                        }
                    }

                    // Emit merged quad
                    const float fx = static_cast<float>(originX + x);
                    const float fy = static_cast<float>(y);
                    const float fz = static_cast<float>(originZ + z);
                    const float fw = static_cast<float>(w);
                    const float fh = static_cast<float>(h);

                    glm::vec3 p0, p1, p2, p3;
                    if (faceIdx == 2) { // +Y
                        const glm::vec3 base(fx, fy + 1.0f, fz + fh);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;

                        if (current.block == BlockId::Water) {
                            p0.y -= 0.12f;
                            p1.y -= 0.12f;
                            p2.y -= 0.12f;
                            p3.y -= 0.12f;
                        }
                    } else { // -Y
                        const glm::vec3 base(fx, fy, fz);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;
                    }

                    const float quadAO[4] = {
                        aoToFloat(current.ao0),
                        aoToFloat(current.ao1),
                        aoToFloat(current.ao2),
                        aoToFloat(current.ao3)
                    };
                    const float quadLight = static_cast<float>(current.light) / 15.0f;

                    if (current.isTransparent) {
                        emitQuad(outTransVertices, outTransIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    } else {
                        emitQuad(outOpaqueVertices, outOpaqueIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    }

                    x += w;
                }
            }
        }
    }

    // 3. Greedy Meshing along Z (+Z and -Z faces)
    // Slice: Z in [0, D-1], 2D grid: X in [0, W-1] (U=W), Y in [0, H-1] (V=H)
    for (int faceIdx = 4; faceIdx < 6; ++faceIdx) {
        const FaceSpec& spec = FACE_SPECS[faceIdx];
        const glm::ivec3& norm = spec.normal;
        const glm::vec3 normF(norm);
        const glm::ivec3 tangI = glm::ivec3(spec.tangent);
        const glm::ivec3 bitangI = glm::ivec3(spec.bitangent);

        for (int z = 0; z < Chunk::D; ++z) {
            std::vector<FaceMask> mask(static_cast<size_t>(Chunk::W) * Chunk::H);

            for (int y = 0; y < Chunk::H; ++y) {
                for (int x = 0; x < Chunk::W; ++x) {
                    const BlockId block = chunk.get(x, y, z);
                    if (isAir(block) || isSpecialModel(block)) continue;

                    const int wx = originX + x;
                    const int wz = originZ + z;
                    const int fx = wx + norm.x;
                    const int fy = y + norm.y;
                    const int fz = wz + norm.z;
                    const BlockId nBlock = world.getBlock(fx, fy, fz);

                    const bool curTrans = isTransparent(block);
                    if (curTrans) {
                        if (nBlock == block || isOpaque(nBlock)) continue;
                    } else {
                        if (isOpaque(nBlock)) continue;
                    }

                    const BlockDef& def = blockDef(block);
                    const TextureTile tile = getFaceTile(block, def, norm);

                    const uint8_t ao0 = computeVertexAO(world, fx, fy, fz, -tangI, -bitangI);
                    const uint8_t ao1 = computeVertexAO(world, fx, fy, fz,  tangI, -bitangI);
                    const uint8_t ao2 = computeVertexAO(world, fx, fy, fz,  tangI,  bitangI);
                    const uint8_t ao3 = computeVertexAO(world, fx, fy, fz, -tangI,  bitangI);

                    const float lightF = world.skyLight(fx, fy, fz);
                    const uint8_t lightU = static_cast<uint8_t>(std::clamp(static_cast<int>(lightF * 15.0f + 0.5f), 0, 15));

                    mask[static_cast<size_t>(y) * Chunk::W + x] = { block, tile, curTrans, ao0, ao1, ao2, ao3, lightU };
                }
            }

            // Greedy merge on mask (U = Chunk::W, V = Chunk::H)
            for (int y = 0; y < Chunk::H; ++y) {
                for (int x = 0; x < Chunk::W;) {
                    const FaceMask current = mask[static_cast<size_t>(y) * Chunk::W + x];
                    if (current.empty()) {
                        x++;
                        continue;
                    }

                    // Compute width along X
                    int w = 1;
                    while (x + w < Chunk::W && mask[static_cast<size_t>(y) * Chunk::W + (x + w)] == current) {
                        w++;
                    }

                    // Compute height along Y
                    int h = 1;
                    bool canExtend = true;
                    while (y + h < Chunk::H && canExtend) {
                        for (int k = 0; k < w; ++k) {
                            if (mask[static_cast<size_t>(y + h) * Chunk::W + (x + k)] != current) {
                                canExtend = false;
                                break;
                            }
                        }
                        if (canExtend) h++;
                    }

                    // Clear visited rectangle
                    for (int dy = 0; dy < h; ++dy) {
                        for (int dx = 0; dx < w; ++dx) {
                            mask[static_cast<size_t>(y + dy) * Chunk::W + (x + dx)] = FaceMask{};
                        }
                    }

                    // Emit merged quad
                    const float fx = static_cast<float>(originX + x);
                    const float fy = static_cast<float>(y);
                    const float fz = static_cast<float>(originZ + z);
                    const float fw = static_cast<float>(w);
                    const float fh = static_cast<float>(h);

                    glm::vec3 p0, p1, p2, p3;
                    if (faceIdx == 4) { // +Z
                        const glm::vec3 base(fx, fy, fz + 1.0f);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;
                    } else { // -Z
                        const glm::vec3 base(fx + fw, fy, fz);
                        p0 = base;
                        p1 = base + fw * spec.tangent;
                        p2 = base + fw * spec.tangent + fh * spec.bitangent;
                        p3 = base + fh * spec.bitangent;
                    }

                    const float quadAO[4] = {
                        aoToFloat(current.ao0),
                        aoToFloat(current.ao1),
                        aoToFloat(current.ao2),
                        aoToFloat(current.ao3)
                    };
                    const float quadLight = static_cast<float>(current.light) / 15.0f;

                    if (current.isTransparent) {
                        emitQuad(outTransVertices, outTransIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    } else {
                        emitQuad(outOpaqueVertices, outOpaqueIndices, p0, p1, p2, p3, normF, fw, fh, current.tile, quadAO, quadLight);
                    }

                    x += w;
                }
            }
        }
    }

    // 4. Emit 3D & cross models for special blocks (e.g. 3D Standing Torches, Wild Tall Grass)
    for (int y = 0; y < Chunk::H; ++y) {
        for (int z = 0; z < Chunk::D; ++z) {
            for (int x = 0; x < Chunk::W; ++x) {
                const BlockId block = chunk.get(x, y, z);
                const float wx = static_cast<float>(originX + x);
                const float wy = static_cast<float>(y);
                const float wz = static_cast<float>(originZ + z);

                if (isTorch(block)) {
                    emitTorch3D(outTransVertices, outTransIndices, wx, wy, wz, block, 1.0f);
                } else if (block == BlockId::TallGrass) {
                    const float light = world.skyLight(static_cast<int>(wx), static_cast<int>(wy), static_cast<int>(wz));
                    emitCrossModel(outTransVertices, outTransIndices, wx, wy, wz, TextureTile::TallGrass, light);
                }
            }
        }
    }
}

} // namespace vox
