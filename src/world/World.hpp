#pragma once
#include "world/Chunk.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "world/TerrainGenerator.hpp"

namespace vox {

struct Village {
    glm::vec3 center{0.0f};
    int radius = 36;
    int templateType = 0;
    std::string name;
};

// ---------------------------------------------------------------------------
// World owns the grid of chunks and is the single entry point for reading and
// writing blocks by *world* coordinates.
//
// Coordinate convention: the world starts at (0, 0, 0) and extends into
// positive X and Z only. This avoids floor-division for negative coordinates.
// y < 0 is treated as solid bedrock so the bottom faces are never drawn.
// ---------------------------------------------------------------------------
class World {
public:
    explicit World(uint32_t seed = config::WORLD_SEED);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void init(int chunksX, int chunksZ, uint32_t seed);
    void generate(WorldType type = WorldType::Default, const ProgressCallback& onProgress = nullptr);
    void initEmptyChunks();

    BlockId getBlock(int wx, int wy, int wz) const;
    void setBlock(int wx, int wy, int wz, BlockId b);

    Chunk* chunkAt(int cx, int cz);
    const Chunk* chunkAt(int cx, int cz) const;

    int chunksX() const { return m_chunksX; }
    int chunksZ() const { return m_chunksZ; }
    int widthBlocks() const { return m_chunksX * Chunk::W; }
    int depthBlocks() const { return m_chunksZ * Chunk::D; }
    int heightBlocks() const { return Chunk::H; }

    int surfaceHeight(int wx, int wz) const;
    float skyLight(int wx, int wy, int wz) const;

    uint32_t seed() const { return m_seed; }
    void setSeed(uint32_t s) { m_seed = s; }

    size_t totalVoxelMemory() const;

    const std::vector<std::unique_ptr<Chunk>>& chunks() const { return m_chunks; }

    const std::vector<Village>& villages() const { return m_villages; }
    std::vector<Village>& villages() { return m_villages; }
    void addVillage(const Village& v) { m_villages.push_back(v); }
    void clearVillages() { m_villages.clear(); }

private:
    int m_chunksX, m_chunksZ;
    uint32_t m_seed;
    std::vector<std::unique_ptr<Chunk>> m_chunks;
    std::vector<Village> m_villages;
};

} // namespace vox
