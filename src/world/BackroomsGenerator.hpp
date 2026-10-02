#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace vox {

class World;
class Chunk;

enum class OchreArchetype : uint8_t {
    StandardCorridors = 0,
    FilingArchives = 1,
    RedstoneConduits = 2,
    DrainageSump = 3,
    FalseOverworld = 4,
};

// ---------------------------------------------------------------------------
// BackroomsGenerator: Procedural Infinite Maze Generation for Level 0 ("The Ochre Annex").
// - 5 Modular Macro-Cell Archetypes (16x16 Chunk Bounds)
// - 2-block thick walls with outer OchrePlaster & procedural Wood inner framing
// - Y=0 Bedrock, Y=1 DampOchreWool, Y=2..4 Corridors, Y=5 ChiseledLimestone, Y>=6 Bedrock seal
// - Resonant Lantern ceiling lighting with blackout zones
// - Anomalous FracturedBedrock portals and VaultHatch exits
// - CondensationFlask caches
// ---------------------------------------------------------------------------
class BackroomsGenerator {
public:
    explicit BackroomsGenerator(uint32_t seed) : m_seed(seed) {}

    void generateChunk(World& world, Chunk& chunk) const;
    glm::vec3 findSafeSpawn(World& world, int startX = 0, int startZ = 0) const;

    static OchreArchetype getArchetype(int cx, int cz, uint32_t seed);
    static bool isWall(int wx, int wz, uint32_t seed);
    static bool isInnerCore(int wx, int wz, uint32_t seed);
    static bool isPillar(int wx, int wz, uint32_t seed);
    static bool isLight(int wx, int wz, uint32_t seed);
    static bool isBlackoutZone(int wx, int wz, uint32_t seed);
    static bool isExitDoor(int wx, int wz, uint32_t seed);
    static bool isAlmondWater(int wx, int wz, uint32_t seed);
    static bool isGlitch(int wx, int wz, uint32_t seed);

private:
    uint32_t m_seed;
};

} // namespace vox
