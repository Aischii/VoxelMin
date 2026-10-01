#pragma once
#include "world/World.hpp"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace vox {

class World;
struct Village;

class VillageGenerator {
public:
    static void generateVillages(World& world, uint32_t seed, std::vector<Village>& outVillages);

private:
    static void buildVillage(World& world, int centerX, int centerZ, int templateType, uint32_t vSeed);

    // Building archetypes
    static void buildWell(World& world, int cx, int cz);
    static void buildTownHall(World& world, int cx, int cz, int rot = 0);
    static void buildCottage(World& world, int cx, int cz, int width, int depth, int rot = 0);
    static void buildWatchtower(World& world, int cx, int cz);
    static void buildFarmPlot(World& world, int cx, int cz, int width, int depth);
    static void buildLampPost(World& world, int x, int z);
    static void buildPath(World& world, int x0, int z0, int x1, int z1);

    // Foundation & helper
    static int prepareFoundation(World& world, int minX, int minZ, int maxX, int maxZ);
};

} // namespace vox
