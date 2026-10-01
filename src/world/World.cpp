#include "world/World.hpp"
#include "world/TerrainGenerator.hpp"

#include <algorithm>
#include <cmath>

namespace vox {

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
    initEmptyChunks();
    TerrainGenerator(m_seed, type).generate(*this, onProgress);
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
    chunk->set(lx, wy, lz, b);
    chunk->dirty = true;

    // A change on a chunk border also affects the neighbour's visible faces.
    if (lx == 0)             { if (Chunk* n = chunkAt(cx - 1, cz)) n->dirty = true; }
    if (lx == Chunk::W - 1)  { if (Chunk* n = chunkAt(cx + 1, cz)) n->dirty = true; }
    if (lz == 0)             { if (Chunk* n = chunkAt(cx, cz - 1)) n->dirty = true; }
    if (lz == Chunk::D - 1)  { if (Chunk* n = chunkAt(cx, cz + 1)) n->dirty = true; }

    // If placing or breaking a light source (torch), dirty all surrounding chunks in light radius
    if (isLightSource(b) || isLightSource(old)) {
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (Chunk* n = chunkAt(cx + dx, cz + dz)) n->dirty = true;
            }
        }
    }
}

int World::surfaceHeight(int wx, int wz) const {
    for (int y = Chunk::H - 1; y >= 0; --y) {
        if (!isAir(getBlock(wx, y, wz))) return y;
    }
    return 0;
}

float World::skyLight(int wx, int wy, int wz) const {
    if (wy < 0) return 0.35f;
    if (wy >= Chunk::H) return 1.0f;

    // Check direct vertical column
    bool directSky = true;
    for (int checkY = wy + 1; checkY < Chunk::H; ++checkY) {
        if (isOpaque(getBlock(wx, checkY, wz))) {
            directSky = false;
            break;
        }
    }
    float maxLight = directSky ? 1.0f : 0.35f;

    // Horizontal light diffusion from nearby columns with open sky (radius 1 and 2)
    if (!directSky) {
        for (int r = 1; r <= 2; ++r) {
            for (int dz = -r; dz <= r; ++dz) {
                for (int dx = -r; dx <= r; ++dx) {
                    if (std::abs(dx) != r && std::abs(dz) != r) continue; // Only ring of radius r

                    const int nx = wx + dx;
                    const int nz = wz + dz;
                    bool nOpen = true;
                    for (int ny = wy; ny < Chunk::H; ++ny) {
                        if (isOpaque(getBlock(nx, ny, nz))) {
                            nOpen = false;
                            break;
                        }
                    }
                    if (nOpen) {
                        const float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                        const float diff = std::max(0.35f, 1.0f - dist * 0.22f);
                        if (diff > maxLight) {
                            maxLight = diff;
                        }
                    }
                }
            }
            if (maxLight >= 0.75f) break;
        }
    }

    // Torch / Block point light contribution (radius 7)
    for (int dy = -6; dy <= 6; ++dy) {
        const int ty = wy + dy;
        if (ty < 0 || ty >= Chunk::H) continue;
        for (int dz = -6; dz <= 6; ++dz) {
            const int tz = wz + dz;
            for (int dx = -6; dx <= 6; ++dx) {
                const int tx = wx + dx;
                if (isLightSource(getBlock(tx, ty, tz))) {
                    const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
                    const float tLight = std::max(0.0f, 1.0f - d * 0.14f);
                    if (tLight > maxLight) {
                        maxLight = tLight;
                    }
                }
            }
        }
    }

    return maxLight;
}

size_t World::totalVoxelMemory() const {
    size_t total = 0;
    for (const auto& chunk : m_chunks) {
        if (chunk) total += chunk->memoryUsage();
    }
    return total;
}

} // namespace vox
