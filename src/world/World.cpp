#include "world/World.hpp"
#include "world/TerrainGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

namespace vox {

namespace {

struct LightNode {
    int x, y, z;
};

const int NEIGHBOR_OFFSETS[6][3] = {
    { 1,  0,  0}, {-1,  0,  0},
    { 0,  1,  0}, { 0, -1,  0},
    { 0,  0,  1}, { 0,  0, -1}
};

} // namespace

World::World(uint32_t seed)
    : m_chunksX(config::WORLD_CHUNKS_X),
      m_chunksZ(config::WORLD_CHUNKS_Z),
      m_seed(seed) {}

World::~World() = default;

void World::init(int chunksX, int chunksZ, uint32_t seed) {
    m_chunksX = chunksX;
    m_chunksZ = chunksZ;
    m_seed = seed;
    m_villages.clear();
    initEmptyChunks();
}

void World::initEmptyChunks() {
    m_chunks.clear();
    m_chunks.reserve(static_cast<size_t>(m_chunksX) * m_chunksZ);
    for (int cz = 0; cz < m_chunksZ; ++cz) {
        for (int cx = 0; cx < m_chunksX; ++cx) {
            m_chunks.push_back(std::make_unique<Chunk>(cx, cz));
        }
    }
}

void World::generate(WorldType type, const ProgressCallback& onProgress) {
    m_generating = true;
    initEmptyChunks();
    TerrainGenerator(m_seed, type).generate(*this, onProgress);
    m_generating = false;
    computeWorldLighting(onProgress);
}

Chunk* World::chunkAt(int cx, int cz) {
    if (cx < 0 || cz < 0 || cx >= m_chunksX || cz >= m_chunksZ) return nullptr;
    return m_chunks[static_cast<size_t>(cz) * m_chunksX + cx].get();
}

const Chunk* World::chunkAt(int cx, int cz) const {
    if (cx < 0 || cz < 0 || cx >= m_chunksX || cz >= m_chunksZ) return nullptr;
    return m_chunks[static_cast<size_t>(cz) * m_chunksX + cx].get();
}

BlockId World::getBlock(int wx, int wy, int wz) const {
    if (wy < 0) return BlockId::Bedrock; // cull bottom faces of the world
    if (wy >= Chunk::H) return BlockId::Air;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return BlockId::Air;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return BlockId::Air;
    return chunk->get(wx - cx * Chunk::W, wy, wz - cz * Chunk::D);
}

void World::setBlock(int wx, int wy, int wz, BlockId b) {
    if (wy < 0 || wy >= Chunk::H) return;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return;

    const int lx = wx - cx * Chunk::W;
    const int lz = wz - cz * Chunk::D;
    const BlockId old = chunk->get(lx, wy, lz);
    if (old == b) return;

    chunk->set(lx, wy, lz, b);
    chunk->dirty = true;

    // A change on a chunk border also affects the neighbour's visible faces.
    if (lx == 0)             { if (Chunk* n = chunkAt(cx - 1, cz)) n->dirty = true; }
    if (lx == Chunk::W - 1)  { if (Chunk* n = chunkAt(cx + 1, cz)) n->dirty = true; }
    if (lz == 0)             { if (Chunk* n = chunkAt(cx, cz - 1)) n->dirty = true; }
    if (lz == Chunk::D - 1)  { if (Chunk* n = chunkAt(cx, cz + 1)) n->dirty = true; }

    if (!m_generating) {
        updateLightAround(wx, wy, wz);
    }
}

uint8_t World::getSunLight(int wx, int wy, int wz) const {
    if (wy >= Chunk::H) return 15;
    if (wy < 0) return 0;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return 15;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return 15;
    return chunk->getSunLight(wx - cx * Chunk::W, wy, wz - cz * Chunk::D);
}

uint8_t World::getBlockLight(int wx, int wy, int wz) const {
    if (wy >= Chunk::H || wy < 0) return 0;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return 0;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return 0;
    return chunk->getBlockLight(wx - cx * Chunk::W, wy, wz - cz * Chunk::D);
}

void World::setSunLight(int wx, int wy, int wz, uint8_t level) {
    if (wy < 0 || wy >= Chunk::H) return;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return;
    chunk->setSunLight(wx - cx * Chunk::W, wy, wz - cz * Chunk::D, level);
}

void World::setBlockLight(int wx, int wy, int wz, uint8_t level) {
    if (wy < 0 || wy >= Chunk::H) return;
    if (wx < 0 || wz < 0 || wx >= widthBlocks() || wz >= depthBlocks()) return;

    const int cx = wx / Chunk::W;
    const int cz = wz / Chunk::D;
    Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return;
    chunk->setBlockLight(wx - cx * Chunk::W, wy, wz - cz * Chunk::D, level);
}

int World::surfaceHeight(int wx, int wz) const {
    for (int y = Chunk::H - 1; y >= 0; --y) {
        if (!isAir(getBlock(wx, y, wz))) return y;
    }
    return 0;
}

float World::skyLight(int wx, int wy, int wz) const {
    const uint8_t sun = getSunLight(wx, wy, wz);
    const uint8_t torch = getBlockLight(wx, wy, wz);
    return std::max(static_cast<float>(sun) / 15.0f, static_cast<float>(torch) / 15.0f);
}

void World::computeWorldLighting(const ProgressCallback& onProgress) {
    std::queue<LightNode> sunQueue;
    std::queue<LightNode> torchQueue;

    const int maxW = widthBlocks();
    const int maxD = depthBlocks();

    if (onProgress) onProgress(0.96f, "Initializing vertical sunlight columns...");

    // 1. Initialize Sunlight Columns directly down from sky
    for (int wz = 0; wz < maxD; ++wz) {
        for (int wx = 0; wx < maxW; ++wx) {
            bool open = true;
            for (int wy = Chunk::H - 1; wy >= 0; --wy) {
                const BlockId b = getBlock(wx, wy, wz);
                if (open && !isOpaque(b)) {
                    setSunLight(wx, wy, wz, 15);
                } else {
                    open = false;
                    setSunLight(wx, wy, wz, 0);
                }
            }
        }
    }

    // 2. Queue only sunlight border nodes (nodes adjacent to non-opaque blocks with sunlight < 14)
    for (int wz = 0; wz < maxD; ++wz) {
        for (int wx = 0; wx < maxW; ++wx) {
            for (int wy = 0; wy < Chunk::H; ++wy) {
                if (getSunLight(wx, wy, wz) == 15) {
                    bool isBorder = false;
                    for (int i = 0; i < 6; ++i) {
                        const int nx = wx + NEIGHBOR_OFFSETS[i][0];
                        const int ny = wy + NEIGHBOR_OFFSETS[i][1];
                        const int nz = wz + NEIGHBOR_OFFSETS[i][2];
                        if (nx >= 0 && nx < maxW && nz >= 0 && nz < maxD && ny >= 0 && ny < Chunk::H) {
                            if (!isOpaque(getBlock(nx, ny, nz)) && getSunLight(nx, ny, nz) < 14) {
                                isBorder = true;
                                break;
                            }
                        }
                    }
                    if (isBorder) {
                        sunQueue.push({wx, wy, wz});
                    }
                }
            }
        }
    }

    if (onProgress) onProgress(0.98f, "Injecting torchlight & block light sources...");

    // 3. Initialize Torch Lights
    for (const auto& chunk : m_chunks) {
        if (!chunk) continue;
        const int ox = chunk->originX();
        const int oz = chunk->originZ();
        for (int y = 0; y < Chunk::H; ++y) {
            for (int z = 0; z < Chunk::D; ++z) {
                for (int x = 0; x < Chunk::W; ++x) {
                    const BlockId b = chunk->get(x, y, z);
                    if (isLightSource(b)) {
                        chunk->setBlockLight(x, y, z, 14);
                        torchQueue.push({ox + x, y, oz + z});
                    }
                }
            }
        }
    }

    // 4. BFS Flood-Fill for Sunlight
    while (!sunQueue.empty()) {
        const LightNode node = sunQueue.front();
        sunQueue.pop();

        const uint8_t curLight = getSunLight(node.x, node.y, node.z);
        if (curLight <= 1) continue;

        const uint8_t nextLight = curLight - 1;

        for (int i = 0; i < 6; ++i) {
            const int nx = node.x + NEIGHBOR_OFFSETS[i][0];
            const int ny = node.y + NEIGHBOR_OFFSETS[i][1];
            const int nz = node.z + NEIGHBOR_OFFSETS[i][2];

            if (nx < 0 || nx >= maxW || nz < 0 || nz >= maxD || ny < 0 || ny >= Chunk::H) continue;

            const BlockId nb = getBlock(nx, ny, nz);
            if (isOpaque(nb)) continue;

            if (getSunLight(nx, ny, nz) < nextLight) {
                setSunLight(nx, ny, nz, nextLight);
                sunQueue.push({nx, ny, nz});
            }
        }
    }

    // 4. BFS Flood-Fill for Torchlight
    while (!torchQueue.empty()) {
        const LightNode node = torchQueue.front();
        torchQueue.pop();

        const uint8_t curLight = getBlockLight(node.x, node.y, node.z);
        if (curLight <= 1) continue;

        const uint8_t nextLight = curLight - 1;

        for (int i = 0; i < 6; ++i) {
            const int nx = node.x + NEIGHBOR_OFFSETS[i][0];
            const int ny = node.y + NEIGHBOR_OFFSETS[i][1];
            const int nz = node.z + NEIGHBOR_OFFSETS[i][2];

            if (nx < 0 || nx >= maxW || nz < 0 || nz >= maxD || ny < 0 || ny >= Chunk::H) continue;

            const BlockId nb = getBlock(nx, ny, nz);
            if (isOpaque(nb)) continue;

            if (getBlockLight(nx, ny, nz) < nextLight) {
                setBlockLight(nx, ny, nz, nextLight);
                torchQueue.push({nx, ny, nz});
            }
        }
    }
}

void World::updateLightAround(int wx, int wy, int wz) {
    const int maxW = widthBlocks();
    const int maxD = depthBlocks();
    const int r = 15;

    const int minX = std::max(0, wx - r);
    const int maxX = std::min(maxW - 1, wx + r);
    const int minZ = std::max(0, wz - r);
    const int maxZ = std::min(maxD - 1, wz + r);
    const int minY = std::max(0, wy - r);
    const int maxY = std::min(Chunk::H - 1, wy + r);

    std::queue<LightNode> sunQueue;
    std::queue<LightNode> torchQueue;

    // Reset local lighting in bounding box
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            // Check direct sky column
            bool openSky = true;
            for (int y = Chunk::H - 1; y >= minY; --y) {
                const BlockId b = getBlock(x, y, z);
                if (openSky && !isOpaque(b)) {
                    if (y >= minY && y <= maxY) {
                        setSunLight(x, y, z, 15);
                        sunQueue.push({x, y, z});
                    }
                } else {
                    openSky = false;
                    if (y >= minY && y <= maxY) {
                        setSunLight(x, y, z, 0);
                    }
                }
            }

            for (int y = minY; y <= maxY; ++y) {
                const BlockId b = getBlock(x, y, z);
                if (isLightSource(b)) {
                    setBlockLight(x, y, z, 14);
                    torchQueue.push({x, y, z});
                } else {
                    setBlockLight(x, y, z, 0);
                }
            }
        }
    }

    // Pull in boundary light from outside the bounding box
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                if (isOpaque(getBlock(x, y, z))) continue;

                for (int i = 0; i < 6; ++i) {
                    const int nx = x + NEIGHBOR_OFFSETS[i][0];
                    const int ny = y + NEIGHBOR_OFFSETS[i][1];
                    const int nz = z + NEIGHBOR_OFFSETS[i][2];

                    if (nx < minX || nx > maxX || nz < minZ || nz > maxZ || ny < minY || ny > maxY) {
                        const uint8_t nSun = getSunLight(nx, ny, nz);
                        if (nSun > 1 && nSun - 1 > getSunLight(x, y, z)) {
                            setSunLight(x, y, z, nSun - 1);
                            sunQueue.push({x, y, z});
                        }
                        const uint8_t nTorch = getBlockLight(nx, ny, nz);
                        if (nTorch > 1 && nTorch - 1 > getBlockLight(x, y, z)) {
                            setBlockLight(x, y, z, nTorch - 1);
                            torchQueue.push({x, y, z});
                        }
                    }
                }
            }
        }
    }

    // Propagate Sunlight
    while (!sunQueue.empty()) {
        const LightNode node = sunQueue.front();
        sunQueue.pop();

        const uint8_t curLight = getSunLight(node.x, node.y, node.z);
        if (curLight <= 1) continue;

        const uint8_t nextLight = curLight - 1;

        for (int i = 0; i < 6; ++i) {
            const int nx = node.x + NEIGHBOR_OFFSETS[i][0];
            const int ny = node.y + NEIGHBOR_OFFSETS[i][1];
            const int nz = node.z + NEIGHBOR_OFFSETS[i][2];

            if (nx < minX || nx > maxX || nz < minZ || nz > maxZ || ny < minY || ny > maxY) continue;

            const BlockId nb = getBlock(nx, ny, nz);
            if (isOpaque(nb)) continue;

            if (getSunLight(nx, ny, nz) < nextLight) {
                setSunLight(nx, ny, nz, nextLight);
                sunQueue.push({nx, ny, nz});
            }
        }
    }

    // Propagate Torchlight
    while (!torchQueue.empty()) {
        const LightNode node = torchQueue.front();
        torchQueue.pop();

        const uint8_t curLight = getBlockLight(node.x, node.y, node.z);
        if (curLight <= 1) continue;

        const uint8_t nextLight = curLight - 1;

        for (int i = 0; i < 6; ++i) {
            const int nx = node.x + NEIGHBOR_OFFSETS[i][0];
            const int ny = node.y + NEIGHBOR_OFFSETS[i][1];
            const int nz = node.z + NEIGHBOR_OFFSETS[i][2];

            if (nx < minX || nx > maxX || nz < minZ || nz > maxZ || ny < minY || ny > maxY) continue;

            const BlockId nb = getBlock(nx, ny, nz);
            if (isOpaque(nb)) continue;

            if (getBlockLight(nx, ny, nz) < nextLight) {
                setBlockLight(nx, ny, nz, nextLight);
                torchQueue.push({nx, ny, nz});
            }
        }
    }

    // Dirty all chunk meshes in affected bounding box
    const int minCx = minX / Chunk::W;
    const int maxCx = maxX / Chunk::W;
    const int minCz = minZ / Chunk::D;
    const int maxCz = maxZ / Chunk::D;

    for (int cz = minCz; cz <= maxCz; ++cz) {
        for (int cx = minCx; cx <= maxCx; ++cx) {
            if (Chunk* chunk = chunkAt(cx, cz)) {
                chunk->dirty = true;
            }
        }
    }
}

size_t World::totalVoxelMemory() const {
    size_t total = 0;
    for (const auto& chunk : m_chunks) {
        if (chunk) total += chunk->memoryUsage();
    }
    return total;
}

} // namespace vox
