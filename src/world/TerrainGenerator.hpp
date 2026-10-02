#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace vox {

class World;
class Chunk;

using ProgressCallback = std::function<void(float progress, const std::string& status)>;

enum class WorldType : uint8_t {
    Default = 0,
    Flat,
    Mountainous,
    Cavernous,
    Island,
    Count
};

inline const char* worldTypeName(WorldType type) {
    switch (type) {
        case WorldType::Flat:        return "Flat";
        case WorldType::Mountainous: return "Mountainous";
        case WorldType::Cavernous:   return "Cavernous";
        case WorldType::Island:      return "Island";
        case WorldType::Default:
        default:                     return "Default";
    }
}

// ---------------------------------------------------------------------------
// Procedural terrain generator with single-chunk and infinite world streaming support.
// ---------------------------------------------------------------------------
class TerrainGenerator {
public:
    explicit TerrainGenerator(uint32_t seed, WorldType type = WorldType::Default)
        : m_seed(seed), m_type(type) {}

    void generate(World& world, const ProgressCallback& onProgress = nullptr) const;
    void generateChunk(World& world, Chunk& chunk, WorldType type) const;
    void generateInitialSpawn(World& world, int radiusInChunks = 2, const ProgressCallback& onProgress = nullptr) const;

    static float hash2(int x, int z, uint32_t seed);
    static float hash3(int x, int y, int z, uint32_t seed);
    static float valueNoise(float x, float z, uint32_t seed);
    static float noise3D(float x, float y, float z, uint32_t seed);
    static float fbm(float x, float z, uint32_t seed);

    static float sampleContinentalness(float wx, float wz, uint32_t seed);
    static float sampleRiver(float wx, float wz, uint32_t seed);
    static float sampleTerrainHeight(float wx, float wz, uint32_t seed, WorldType type);

private:
    void plantTreeInChunk(Chunk& chunk, int originX, int originZ, int x, int groundY, int z, float cont) const;

    uint32_t m_seed;
    WorldType m_type = WorldType::Default;
};

} // namespace vox
