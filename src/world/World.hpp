#pragma once
#include "world/Chunk.hpp"
#include "world/Dimension.hpp"
#include "world/TerrainGenerator.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace vox {

struct Village {
    glm::vec3 center{0.0f};
    int radius = 36;
    int templateType = 0;
    std::string name;
};

// ---------------------------------------------------------------------------
// Chunk Coordinates and 64-bit SplitMix Hash for Infinite World Map
// ---------------------------------------------------------------------------
struct ChunkCoord {
    int x = 0;
    int z = 0;
    bool operator==(const ChunkCoord& o) const noexcept { return x == o.x && z == o.z; }
    bool operator!=(const ChunkCoord& o) const noexcept { return !(*this == o); }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const noexcept {
        uint64_t a = static_cast<uint64_t>(static_cast<uint32_t>(c.x));
        uint64_t b = static_cast<uint64_t>(static_cast<uint32_t>(c.z));
        uint64_t key = (a << 32) | b;
        key = (key ^ (key >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        key = (key ^ (key >> 27)) * UINT64_C(0x94d049bb133111eb);
        key = key ^ (key >> 31);
        return static_cast<size_t>(key);
    }
};

inline int blockToChunk(int c) {
    return (c >= 0) ? (c / 16) : ((c - 15) / 16);
}

inline int blockToLocal(int c) {
    int l = c % 16;
    return (l < 0) ? (l + 16) : l;
}

using ChunkMap = std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash>;

// ---------------------------------------------------------------------------
// World owns the dynamic hash map of chunks and is the single entry point for
// reading and writing blocks by *world* coordinates (-inf to +inf).
// ---------------------------------------------------------------------------
class World {
public:
    explicit World(uint32_t seed = config::WORLD_SEED);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void init(uint32_t seed);
    void init(int chunksX, int chunksZ, uint32_t seed);
    void generate(WorldType type = WorldType::Default, const ProgressCallback& onProgress = nullptr);
    void generateInitialSpawn(WorldType type = WorldType::Default, int radiusInChunks = 2, const ProgressCallback& onProgress = nullptr);

    BlockId getBlock(int wx, int wy, int wz) const;
    void setBlock(int wx, int wy, int wz, BlockId b);

    Chunk* chunkAt(int cx, int cz);
    const Chunk* chunkAt(int cx, int cz) const;
    Chunk* getOrCreateChunk(int cx, int cz);

    void generateSingleChunk(int cx, int cz, WorldType type);
    void updateStreaming(const glm::vec3& playerPos, int viewDistanceChunks, WorldType type);

    int chunksX() const { return m_chunksX; }
    int chunksZ() const { return m_chunksZ; }
    int widthBlocks() const { return m_chunksX * Chunk::W; }
    int depthBlocks() const { return m_chunksZ * Chunk::D; }
    int heightBlocks() const { return Chunk::H; }

    int surfaceHeight(int wx, int wz) const;
    float skyLight(int wx, int wy, int wz) const;

    uint8_t getSunLight(int wx, int wy, int wz) const;
    uint8_t getBlockLight(int wx, int wy, int wz) const;
    void setSunLight(int wx, int wy, int wz, uint8_t level);
    void setBlockLight(int wx, int wy, int wz, uint8_t level);

    void computeWorldLighting(const ProgressCallback& onProgress = nullptr);
    void computeChunkLighting(Chunk& chunk);
    void updateLightAround(int wx, int wy, int wz);

    // Ecology & Fluid simulation
    void tickEcology(const glm::ivec3& playerPos, int radius,
                     const std::function<void(const glm::vec3&, BlockId)>& onDrop = nullptr);
    void tickFluids(const glm::ivec3& playerPos, int radius);
    void scheduleFluidUpdate(int wx, int wy, int wz);
    void setFluidBlock(int wx, int wy, int wz, BlockId b);

    uint32_t seed() const { return m_seed; }
    void setSeed(uint32_t s) { m_seed = s; }

    size_t totalVoxelMemory() const;

    const ChunkMap& chunkMap() const { return m_chunks; }
    const std::vector<Chunk*>& loadedChunks() const { return m_loadedList; }
    const std::vector<std::unique_ptr<Chunk>>& chunks() const;

    const std::vector<Village>& villages() const { return m_villages; }
    std::vector<Village>& villages() { return m_villages; }
    void addVillage(const Village& v) { m_villages.push_back(v); }
    void clearVillages() { m_villages.clear(); }

    bool isGenerating() const { return m_generating; }
    void setGenerating(bool g) { m_generating = g; }
    void rebuildLoadedList();

    // Dimensions
    DimensionId currentDimension() const { return m_currentDimension; }
    void setCurrentDimension(DimensionId dim) { m_currentDimension = dim; }
    void switchDimension(DimensionId newDim, const glm::vec3& newPos);

    // Persistent Chunk Cache
    bool hasSavedChunk(int cx, int cz) const;
    void saveChunkToCache(const Chunk& chunk);
    void cacheChunkData(int cx, int cz, const std::vector<RLERun>& runs);
    const std::unordered_map<ChunkCoord, std::vector<RLERun>, ChunkCoordHash>& savedChunkCache() const {
        return (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    }
    std::unordered_map<ChunkCoord, std::vector<RLERun>, ChunkCoordHash>& savedChunkCache() {
        return (m_currentDimension == DimensionId::Backrooms) ? m_backroomsCache : m_overworldCache;
    }
    void syncAllChunksToCache();

private:

    int m_chunksX;
    int m_chunksZ;
    uint32_t m_seed;
    bool m_generating = false;
    DimensionId m_currentDimension = DimensionId::Overworld;
    ChunkMap m_chunks;
    std::vector<Chunk*> m_loadedList;
    mutable std::vector<std::unique_ptr<Chunk>> m_dummyChunks;
    std::vector<Village> m_villages;
    std::vector<glm::ivec3> m_activeFluids;
    std::unordered_map<ChunkCoord, std::vector<RLERun>, ChunkCoordHash> m_overworldCache;
    std::unordered_map<ChunkCoord, std::vector<RLERun>, ChunkCoordHash> m_backroomsCache;
};

} // namespace vox
