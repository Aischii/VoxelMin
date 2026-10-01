#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace vox {

class World;

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
// Procedural terrain generator.
// ---------------------------------------------------------------------------
class TerrainGenerator {
public:
    explicit TerrainGenerator(uint32_t seed, WorldType type = WorldType::Default)
        : m_seed(seed), m_type(type) {}

    void generate(World& world, const ProgressCallback& onProgress = nullptr) const;

private:
    void plantTree(World& world, int x, int groundY, int z) const;

    static float hash2(int x, int z, uint32_t seed);
    static float hash3(int x, int y, int z, uint32_t seed);
    static float valueNoise(float x, float z, uint32_t seed);
    static float noise3D(float x, float y, float z, uint32_t seed);
    static float fbm(float x, float z, uint32_t seed);

    uint32_t m_seed;
    WorldType m_type = WorldType::Default;
};

} // namespace vox
