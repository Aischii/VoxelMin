#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace vox {

class World;
class Chunk;

// ---------------------------------------------------------------------------
// BackroomsGenerator: Procedural Infinite Maze Generation for Level 0 ("The Yellow Hell").
// - Infinite mono-yellow wallpaper maze partitions
// - Damp stained carpet floor (Y=1)
// - Acoustic ceiling tiles with buzzing fluorescent light panels (Y=5)
// - Office support pillars (2x2 & 3x3), partitioned rooms, open column halls
// - Dark blackout anomaly zones (sparse/broken lights)
// - Anomalous Reality Glitches and Emergency Fire Exit doors to escape back to Overworld
// - Almond Water supply item caches
// ---------------------------------------------------------------------------
class BackroomsGenerator {
public:
    explicit BackroomsGenerator(uint32_t seed) : m_seed(seed) {}

    void generateChunk(World& world, Chunk& chunk) const;
    glm::vec3 findSafeSpawn(World& world, int startX = 0, int startZ = 0) const;

    static bool isWall(int wx, int wz, uint32_t seed);
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
