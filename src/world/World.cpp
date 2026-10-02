#include "world/World.hpp"
#include "world/TerrainGenerator.hpp"
#include "world/BackroomsGenerator.hpp"

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
      m_seed(seed),
      m_currentDimension(DimensionId::Overworld) {}

World::~World() = default;

void World::rebuildLoadedList() {
    m_loadedList.clear();
    m_loadedList.reserve(m_chunks.size());
    for (auto& pair : m_chunks) {
        if (pair.second) {
            m_loadedList.push_back(pair.second.get());
        }
    }
}

const std::vector<std::unique_ptr<Chunk>>& World::chunks() const {
    // For any legacy caller expecting vector of unique_ptr
    m_dummyChunks.clear();
    return m_dummyChunks;
}

void World::init(uint32_t seed) {
    m_seed = seed;
    m_currentDimension = DimensionId::Overworld;
    m_villages.clear();
    m_chunks.clear();
    m_loadedList.clear();
    m_activeFluids.clear();
    m_overworldCache.clear();
    m_backroomsCache.clear();
}

void World::init(int chunksX, int chunksZ, uint32_t seed) {
    m_chunksX = chunksX;
    m_chunksZ = chunksZ;
    m_seed = seed;
    m_currentDimension = DimensionId::Overworld;
    m_villages.clear();
    m_chunks.clear();
    m_loadedList.clear();
    m_activeFluids.clear();
    m_overworldCache.clear();
    m_backroomsCache.clear();
}

void World::saveChunkToCache(const Chunk& chunk) {
    auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    cache[ChunkCoord{chunk.chunkX(), chunk.chunkZ()}] = chunk.rleCompress();
}

void World::cacheChunkData(int cx, int cz, const std::vector<RLERun>& runs) {
    auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    cache[ChunkCoord{cx, cz}] = runs;
}

bool World::hasSavedChunk(int cx, int cz) const {
    const auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    return cache.find(ChunkCoord{cx, cz}) != cache.end();
}

void World::syncAllChunksToCache() {
    auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    for (const auto& pair : m_chunks) {
        if (pair.second && pair.second->terrainGenerated) {
            cache[pair.first] = pair.second->rleCompress();
        }
    }
}

void World::switchDimension(DimensionId newDim, const glm::vec3& newPos) {
    if (m_currentDimension == newDim) return;

    m_generating = true;
    syncAllChunksToCache();

    m_chunks.clear();
    m_loadedList.clear();
    m_activeFluids.clear();

    m_currentDimension = newDim;

    const int targetCx = blockToChunk(static_cast<int>(std::floor(newPos.x)));
    const int targetCz = blockToChunk(static_cast<int>(std::floor(newPos.z)));
    const int radius = 2;
    for (int cz = targetCz - radius; cz <= targetCz + radius; ++cz) {
        for (int cx = targetCx - radius; cx <= targetCx + radius; ++cx) {
            generateSingleChunk(cx, cz, WorldType::Default);
        }
    }

    rebuildLoadedList();
    computeWorldLighting();
    m_generating = false;
}

void World::generate(WorldType type, const ProgressCallback& onProgress) {
    m_generating = true;
    m_chunks.clear();
    m_loadedList.clear();
    generateInitialSpawn(type, 2, onProgress);
    m_generating = false;
}

void World::generateInitialSpawn(WorldType type, int radiusInChunks, const ProgressCallback& onProgress) {
    m_generating = true;
    m_chunks.clear();
    m_loadedList.clear();
    if (m_currentDimension == DimensionId::Backrooms) {
        const int radius = std::max(1, radiusInChunks);
        for (int cz = -radius; cz <= radius; ++cz) {
            for (int cx = -radius; cx <= radius; ++cx) {
                generateSingleChunk(cx, cz, type);
            }
        }
    } else {
        TerrainGenerator(m_seed, type).generateInitialSpawn(*this, radiusInChunks, onProgress);
    }
    rebuildLoadedList();
    syncAllChunksToCache();
    m_generating = false;
}

Chunk* World::chunkAt(int cx, int cz) {
    auto it = m_chunks.find(ChunkCoord{cx, cz});
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Chunk* World::chunkAt(int cx, int cz) const {
    auto it = m_chunks.find(ChunkCoord{cx, cz});
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* World::getOrCreateChunk(int cx, int cz) {
    ChunkCoord coord{cx, cz};
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    auto chunk = std::make_unique<Chunk>(cx, cz);
    Chunk* ptr = chunk.get();
    m_chunks.emplace(coord, std::move(chunk));
    m_loadedList.push_back(ptr);
    return ptr;
}

void World::generateSingleChunk(int cx, int cz, WorldType type) {
    Chunk* chunk = getOrCreateChunk(cx, cz);
    if (chunk->terrainGenerated) return;

    // If chunk was previously generated or loaded from disk, restore its exact blocks directly!
    auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    auto it = cache.find(ChunkCoord{cx, cz});
    if (it != cache.end()) {
        chunk->decompressRle(it->second);
    } else {
        if (m_currentDimension == DimensionId::Backrooms) {
            BackroomsGenerator(m_seed).generateChunk(*this, *chunk);
        } else {
            TerrainGenerator(m_seed, type).generateChunk(*this, *chunk, type);
        }
        cache[ChunkCoord{cx, cz}] = chunk->rleCompress();
    }
    chunk->terrainGenerated = true;

    computeChunkLighting(*chunk);
    chunk->dirty = true;

    // Neighbor chunks border dirtying
    if (Chunk* n = chunkAt(cx - 1, cz)) n->dirty = true;
    if (Chunk* n = chunkAt(cx + 1, cz)) n->dirty = true;
    if (Chunk* n = chunkAt(cx, cz - 1)) n->dirty = true;
    if (Chunk* n = chunkAt(cx, cz + 1)) n->dirty = true;
}

void World::updateStreaming(const glm::vec3& playerPos, int viewDistanceChunks, WorldType type) {
    if (m_generating) return;

    const int playerCx = blockToChunk(static_cast<int>(std::floor(playerPos.x)));
    const int playerCz = blockToChunk(static_cast<int>(std::floor(playerPos.z)));
    const int maxGenRadius = std::max(2, viewDistanceChunks + 2);
    const int maxGenDistSq = maxGenRadius * maxGenRadius;

    // 1. Collect missing chunks or ungenerated chunks within view distance + 2
    struct ChunkCandidate {
        int cx, cz;
        int distSq;
    };
    std::vector<ChunkCandidate> candidates;

    for (int cz = playerCz - maxGenRadius; cz <= playerCz + maxGenRadius; ++cz) {
        for (int cx = playerCx - maxGenRadius; cx <= playerCx + maxGenRadius; ++cx) {
            const int dx = cx - playerCx;
            const int dz = cz - playerCz;
            const int distSq = dx * dx + dz * dz;
            if (distSq <= maxGenDistSq) {
                auto it = m_chunks.find(ChunkCoord{cx, cz});
                if (it == m_chunks.end() || !it->second->terrainGenerated) {
                    candidates.push_back({cx, cz, distSq});
                }
            }
        }
    }

    if (!candidates.empty()) {
        // Sort closest first
        std::sort(candidates.begin(), candidates.end(), [](const ChunkCandidate& a, const ChunkCandidate& b) {
            return a.distSq < b.distSq;
        });

        // Budget: generate/load up to 6 chunks per frame to keep up with player movement
        const size_t generateCount = std::min<size_t>(6, candidates.size());
        for (size_t i = 0; i < generateCount; ++i) {
            generateSingleChunk(candidates[i].cx, candidates[i].cz, type);
        }
    }

    // 2. Unload chunks beyond view distance + 2: persist modified voxels before freeing RAM
    const int unloadDist = maxGenRadius + 1;
    const int unloadDistSq = unloadDist * unloadDist;
    std::vector<ChunkCoord> toUnload;

    for (const auto& pair : m_chunks) {
        const int dx = pair.first.x - playerCx;
        const int dz = pair.first.z - playerCz;
        if (dx * dx + dz * dz > unloadDistSq) {
            toUnload.push_back(pair.first);
        }
    }

    if (!toUnload.empty()) {
        auto& cache = (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
        for (const auto& coord : toUnload) {
            auto it = m_chunks.find(coord);
            if (it != m_chunks.end()) {
                if (it->second && it->second->terrainGenerated) {
                    cache[coord] = it->second->rleCompress();
                }
                m_chunks.erase(it);
            }
        }
        rebuildLoadedList();
    }
}

BlockId World::getBlock(int wx, int wy, int wz) const {
    if (wy < 0) return BlockId::Bedrock;
    if (wy >= Chunk::H) return BlockId::Air;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return BlockId::Air;

    return chunk->get(blockToLocal(wx), wy, blockToLocal(wz));
}

void World::setFluidBlock(int wx, int wy, int wz, BlockId b) {
    if (wy < 0 || wy >= Chunk::H) return;
    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    Chunk* chunk = getOrCreateChunk(cx, cz);

    const int lx = blockToLocal(wx);
    const int lz = blockToLocal(wz);
    const BlockId old = chunk->get(lx, wy, lz);
    if (old == b) return;

    chunk->set(lx, wy, lz, b);
    chunk->dirty = true;

    if (lx == 0)             { if (Chunk* n = chunkAt(cx - 1, cz)) n->dirty = true; }
    if (lx == Chunk::W - 1)  { if (Chunk* n = chunkAt(cx + 1, cz)) n->dirty = true; }
    if (lz == 0)             { if (Chunk* n = chunkAt(cx, cz - 1)) n->dirty = true; }
    if (lz == Chunk::D - 1)  { if (Chunk* n = chunkAt(cx, cz + 1)) n->dirty = true; }
}

void World::scheduleFluidUpdate(int wx, int wy, int wz) {
    if (wy < 0 || wy >= Chunk::H) return;
    if (m_generating) return;
    if (m_activeFluids.size() < 2048) {
        m_activeFluids.push_back(glm::ivec3(wx, wy, wz));
    }
}

void World::setBlock(int wx, int wy, int wz, BlockId b) {
    if (wy < 0 || wy >= Chunk::H) return;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    Chunk* chunk = getOrCreateChunk(cx, cz);

    const int lx = blockToLocal(wx);
    const int lz = blockToLocal(wz);
    const BlockId old = chunk->get(lx, wy, lz);
    if (old == b) return;

    chunk->set(lx, wy, lz, b);
    chunk->dirty = true;

    if (lx == 0)             { if (Chunk* n = chunkAt(cx - 1, cz)) n->dirty = true; }
    if (lx == Chunk::W - 1)  { if (Chunk* n = chunkAt(cx + 1, cz)) n->dirty = true; }
    if (lz == 0)             { if (Chunk* n = chunkAt(cx, cz - 1)) n->dirty = true; }
    if (lz == Chunk::D - 1)  { if (Chunk* n = chunkAt(cx, cz + 1)) n->dirty = true; }

    if (!m_generating) {
        if (b == BlockId::Water) {
            scheduleFluidUpdate(wx, wy, wz);
        } else if (b == BlockId::Air) {
            if (wy + 1 < Chunk::H) {
                const BlockId above = getBlock(wx, wy + 1, wz);
                if (above == BlockId::TallGrass) {
                    setBlock(wx, wy + 1, wz, BlockId::Air);
                }
            }
            // Trigger neighbor fluids to flow into newly emptied space
            for (int i = 0; i < 6; ++i) {
                const int nx = wx + NEIGHBOR_OFFSETS[i][0];
                const int ny = wy + NEIGHBOR_OFFSETS[i][1];
                const int nz = wz + NEIGHBOR_OFFSETS[i][2];
                if (ny >= 0 && ny < Chunk::H && getBlock(nx, ny, nz) == BlockId::Water) {
                    scheduleFluidUpdate(nx, ny, nz);
                }
            }
        }
        updateLightAround(wx, wy, wz);
    }
}

void World::tickEcology(const glm::ivec3& playerPos, int radius,
                        const std::function<void(const glm::vec3&, BlockId)>& onDrop) {
    if (m_generating) return;

    for (int i = 0; i < 24; ++i) {
        const int rx = playerPos.x + (std::rand() % (2 * radius + 1) - radius);
        const int rz = playerPos.z + (std::rand() % (2 * radius + 1) - radius);
        const int ry = playerPos.y + (std::rand() % 21 - 10);

        if (ry <= 0 || ry >= Chunk::H - 1) continue;

        const BlockId blk = getBlock(rx, ry, rz);
        if (blk == BlockId::TallGrass) {
            const BlockId below = getBlock(rx, ry - 1, rz);
            if (below != BlockId::Grass && below != BlockId::Dirt && below != BlockId::DirtPath) {
                setBlock(rx, ry, rz, BlockId::Air);
                if (onDrop) onDrop(glm::vec3(rx + 0.5f, ry + 0.5f, rz + 0.5f), blk);
            }
        } else if (blk == BlockId::Dirt) {
            const BlockId above = getBlock(rx, ry + 1, rz);
            if (above == BlockId::Air && getSunLight(rx, ry + 1, rz) >= 4) {
                bool hasGrassNeighbor = false;
                for (int dx = -1; dx <= 1 && !hasGrassNeighbor; ++dx) {
                    for (int dy = -1; dy <= 1 && !hasGrassNeighbor; ++dy) {
                        for (int dz = -1; dz <= 1 && !hasGrassNeighbor; ++dz) {
                            if (getBlock(rx + dx, ry + dy, rz + dz) == BlockId::Grass) {
                                hasGrassNeighbor = true;
                            }
                        }
                    }
                }
                if (hasGrassNeighbor) {
                    setBlock(rx, ry, rz, BlockId::Grass);
                }
            }
        } else if (blk == BlockId::Grass) {
            const BlockId above = getBlock(rx, ry + 1, rz);
            if (above != BlockId::Air && isOpaque(above)) {
                setBlock(rx, ry, rz, BlockId::Dirt);
            }
        }
    }
}

void World::tickFluids(const glm::ivec3& playerPos, int radius) {
    if (m_generating) return;

    // 1. Process active queued fluids (immediate cascades & expansions)
    if (!m_activeFluids.empty()) {
        std::vector<glm::ivec3> currentBatch = std::move(m_activeFluids);
        m_activeFluids.clear();
        m_activeFluids.reserve(64);

        const size_t processLimit = std::min<size_t>(currentBatch.size(), 64);
        for (size_t k = 0; k < processLimit; ++k) {
            const glm::ivec3 pos = currentBatch[k];
            if (pos.y <= 0 || pos.y >= Chunk::H) continue;
            if (getBlock(pos.x, pos.y, pos.z) != BlockId::Water) continue;

            // 1a. Downward cascade: if air or tall grass below, fall straight down
            const BlockId below = getBlock(pos.x, pos.y - 1, pos.z);
            if (below == BlockId::Air || below == BlockId::TallGrass) {
                setFluidBlock(pos.x, pos.y - 1, pos.z, BlockId::Water);
                scheduleFluidUpdate(pos.x, pos.y - 1, pos.z);
                continue;
            }

            // 1b. Horizontal expansion: if supported by solid ground or water, spread in 4 directions
            if (isSolid(below) || below == BlockId::Water) {
                const int offsets[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
                for (int j = 0; j < 4; ++j) {
                    const int nx = pos.x + offsets[j][0];
                    const int nz = pos.z + offsets[j][1];
                    const BlockId nBlock = getBlock(nx, pos.y, nz);
                    if (nBlock == BlockId::Air || nBlock == BlockId::TallGrass) {
                        setFluidBlock(nx, pos.y, nz, BlockId::Water);
                        scheduleFluidUpdate(nx, pos.y, nz);
                    }
                }
            }
        }
    }

    // 2. Continuous environmental fluid sweep around player
    for (int i = 0; i < 32; ++i) {
        const int rx = playerPos.x + (std::rand() % (2 * radius + 1) - radius);
        const int rz = playerPos.z + (std::rand() % (2 * radius + 1) - radius);
        const int ry = playerPos.y + (std::rand() % 31 - 15);

        if (ry <= 1 || ry >= Chunk::H - 1) continue;

        const BlockId blk = getBlock(rx, ry, rz);
        if (blk == BlockId::Water) {
            const BlockId below = getBlock(rx, ry - 1, rz);
            if (below == BlockId::Air || below == BlockId::TallGrass) {
                setFluidBlock(rx, ry - 1, rz, BlockId::Water);
                scheduleFluidUpdate(rx, ry - 1, rz);
            } else if (isSolid(below) || below == BlockId::Water) {
                const int offsets[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
                for (int j = 0; j < 4; ++j) {
                    const int nx = rx + offsets[j][0];
                    const int nz = rz + offsets[j][1];
                    const BlockId nBlock = getBlock(nx, ry, nz);
                    if (nBlock == BlockId::Air || nBlock == BlockId::TallGrass) {
                        setFluidBlock(nx, ry, nz, BlockId::Water);
                        scheduleFluidUpdate(nx, ry, nz);
                        break;
                    }
                }
            }
        }
    }
}

uint8_t World::getSunLight(int wx, int wy, int wz) const {
    if (wy >= Chunk::H) return 15;
    if (wy < 0) return 0;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return (wy >= 26) ? 15 : 0;
    return chunk->getSunLight(blockToLocal(wx), wy, blockToLocal(wz));
}

uint8_t World::getBlockLight(int wx, int wy, int wz) const {
    if (wy >= Chunk::H || wy < 0) return 0;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return 0;
    return chunk->getBlockLight(blockToLocal(wx), wy, blockToLocal(wz));
}

void World::setSunLight(int wx, int wy, int wz, uint8_t level) {
    if (wy < 0 || wy >= Chunk::H) return;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return;
    chunk->setSunLight(blockToLocal(wx), wy, blockToLocal(wz), level);
}

void World::setBlockLight(int wx, int wy, int wz, uint8_t level) {
    if (wy < 0 || wy >= Chunk::H) return;

    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return;
    chunk->setBlockLight(blockToLocal(wx), wy, blockToLocal(wz), level);
}

int World::surfaceHeight(int wx, int wz) const {
    const int cx = blockToChunk(wx);
    const int cz = blockToChunk(wz);
    const Chunk* chunk = chunkAt(cx, cz);
    if (!chunk) return 0;

    const int lx = blockToLocal(wx);
    const int lz = blockToLocal(wz);
    for (int y = Chunk::H - 1; y >= 0; --y) {
        if (!isAir(chunk->get(lx, y, lz))) return y;
    }
    return 0;
}

float World::skyLight(int wx, int wy, int wz) const {
    const uint8_t sun = getSunLight(wx, wy, wz);
    const uint8_t torch = getBlockLight(wx, wy, wz);
    return std::max(static_cast<float>(sun) / 15.0f, static_cast<float>(torch) / 15.0f);
}

void World::computeChunkLighting(Chunk& chunk) {
    // 1. Sunlight
    if (m_currentDimension == DimensionId::Backrooms) {
        const int ox = chunk.originX();
        const int oz = chunk.originZ();
        for (int lz = 0; lz < Chunk::D; ++lz) {
            const int wz = oz + lz;
            for (int lx = 0; lx < Chunk::W; ++lx) {
                const int wx = ox + lx;
                const bool dark = BackroomsGenerator::isBlackoutZone(wx, wz, m_seed);
                const uint8_t ambientLevel = dark ? 0 : 14;
                for (int y = 0; y < Chunk::H; ++y) {
                    const BlockId b = chunk.get(lx, y, lz);
                    if (y >= 1 && y <= 5 && !isOpaque(b)) {
                        chunk.setSunLight(lx, y, lz, ambientLevel);
                    } else {
                        chunk.setSunLight(lx, y, lz, 0);
                    }
                }
            }
        }
    } else {
        for (int lz = 0; lz < Chunk::D; ++lz) {
            for (int lx = 0; lx < Chunk::W; ++lx) {
                bool open = true;
                for (int y = Chunk::H - 1; y >= 0; --y) {
                    const BlockId b = chunk.get(lx, y, lz);
                    if (open && !isOpaque(b)) {
                        chunk.setSunLight(lx, y, lz, 15);
                    } else {
                        open = false;
                        chunk.setSunLight(lx, y, lz, 0);
                    }
                }
            }
        }
    }

    // 2. Torches / Fluorescent Light Sources
    for (int y = 0; y < Chunk::H; ++y) {
        for (int lz = 0; lz < Chunk::D; ++lz) {
            for (int lx = 0; lx < Chunk::W; ++lx) {
                const BlockId b = chunk.get(lx, y, lz);
                if (isLightSource(b)) {
                    chunk.setBlockLight(lx, y, lz, emittedLight(b));
                }
            }
        }
    }
}

void World::computeWorldLighting(const ProgressCallback& onProgress) {
    std::queue<LightNode> sunQueue;
    std::queue<LightNode> torchQueue;

    if (onProgress) onProgress(0.86f, "Initializing vertical sunlight columns...");

    // 1. Initialize Sunlight Columns for all loaded chunks
    for (Chunk* chunk : m_loadedList) {
        if (!chunk) continue;
        if (m_currentDimension == DimensionId::Backrooms) {
            const int ox = chunk->originX();
            const int oz = chunk->originZ();
            for (int lz = 0; lz < Chunk::D; ++lz) {
                const int wz = oz + lz;
                for (int lx = 0; lx < Chunk::W; ++lx) {
                    const int wx = ox + lx;
                    const bool dark = BackroomsGenerator::isBlackoutZone(wx, wz, m_seed);
                    const uint8_t ambientLevel = dark ? 0 : 14;
                    for (int y = 0; y < Chunk::H; ++y) {
                        const BlockId b = chunk->get(lx, y, lz);
                        if (y >= 1 && y <= 5 && !isOpaque(b)) {
                            chunk->setSunLight(lx, y, lz, ambientLevel);
                        } else {
                            chunk->setSunLight(lx, y, lz, 0);
                        }
                    }
                }
            }
        } else {
            for (int lz = 0; lz < Chunk::D; ++lz) {
                for (int lx = 0; lx < Chunk::W; ++lx) {
                    bool open = true;
                    for (int y = Chunk::H - 1; y >= 0; --y) {
                        const BlockId b = chunk->get(lx, y, lz);
                        if (open && !isOpaque(b)) {
                            chunk->setSunLight(lx, y, lz, 15);
                        } else {
                            open = false;
                            chunk->setSunLight(lx, y, lz, 0);
                        }
                    }
                }
            }
        }
    }

    if (onProgress) onProgress(0.90f, "Queueing light border nodes...");

    // 2. Queue sunlight border nodes
    for (Chunk* chunk : m_loadedList) {
        if (!chunk) continue;
        const int ox = chunk->originX();
        const int oz = chunk->originZ();

        for (int lz = 0; lz < Chunk::D; ++lz) {
            const int wz = oz + lz;
            for (int lx = 0; lx < Chunk::W; ++lx) {
                const int wx = ox + lx;
                for (int wy = 0; wy < Chunk::H; ++wy) {
                    if (chunk->getSunLight(lx, wy, lz) == 15) {
                        bool isBorder = false;
                        for (int i = 0; i < 6; ++i) {
                            const int nx = wx + NEIGHBOR_OFFSETS[i][0];
                            const int ny = wy + NEIGHBOR_OFFSETS[i][1];
                            const int nz = wz + NEIGHBOR_OFFSETS[i][2];
                            if (ny >= 0 && ny < Chunk::H) {
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
    }

    if (onProgress) onProgress(0.94f, "Injecting torchlight sources...");

    // 3. Initialize Torch Lights
    for (Chunk* chunk : m_loadedList) {
        if (!chunk) continue;
        const int ox = chunk->originX();
        const int oz = chunk->originZ();
        for (int y = 0; y < Chunk::H; ++y) {
            for (int z = 0; z < Chunk::D; ++z) {
                for (int x = 0; x < Chunk::W; ++x) {
                    const BlockId b = chunk->get(x, y, z);
                    if (isLightSource(b)) {
                        chunk->setBlockLight(x, y, z, emittedLight(b));
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

            if (ny < 0 || ny >= Chunk::H) continue;

            const BlockId nb = getBlock(nx, ny, nz);
            if (isOpaque(nb)) continue;

            if (getSunLight(nx, ny, nz) < nextLight) {
                setSunLight(nx, ny, nz, nextLight);
                sunQueue.push({nx, ny, nz});
            }
        }
    }

    // 5. BFS Flood-Fill for Torchlight
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

            if (ny < 0 || ny >= Chunk::H) continue;

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
    const int r = 8;
    const int minX = wx - r;
    const int maxX = wx + r;
    const int minZ = wz - r;
    const int maxZ = wz + r;
    const int minY = std::max(0, wy - r);
    const int maxY = std::min(Chunk::H - 1, wy + r);

    std::queue<LightNode> sunQueue;
    std::queue<LightNode> torchQueue;

    // Reset local lighting in bounding box
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
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

    // Dirty affected chunk meshes
    const int minCx = blockToChunk(minX);
    const int maxCx = blockToChunk(maxX);
    const int minCz = blockToChunk(minZ);
    const int maxCz = blockToChunk(maxZ);

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
    for (const auto& pair : m_chunks) {
        if (pair.second) total += pair.second->memoryUsage();
    }
    return total;
}

} // namespace vox
