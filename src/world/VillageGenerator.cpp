#include "world/VillageGenerator.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace vox {
namespace {

uint32_t hashCoord(int x, int z, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393U +
                 static_cast<uint32_t>(z) * 668265263U +
                 seed * 362437U;
    h = (h ^ (h >> 13)) * 1274126177U;
    h ^= h >> 16;
    return h;
}

void safeSet(World& world, int x, int y, int z, BlockId b) {
    if (y >= 0 && y < Chunk::H) {
        world.setBlock(x, y, z, b);
    }
}

BlockId safeGet(const World& world, int x, int y, int z) {
    if (y >= 0 && y < Chunk::H) {
        return world.getBlock(x, y, z);
    }
    return BlockId::Air;
}

} // namespace

int VillageGenerator::prepareFoundation(World& world, int minX, int minZ, int maxX, int maxZ) {
    // Determine the average surface elevation across the footprint
    int sumY = 0;
    int count = 0;
    int minY = Chunk::H;
    int maxY = 0;

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            int sy = world.surfaceHeight(x, z);
            sumY += sy;
            count++;
            if (sy < minY) minY = sy;
            if (sy > maxY) maxY = sy;
        }
    }

    if (count == 0) return 28;
    int targetFloorY = sumY / count;
    if (targetFloorY < 27) targetFloorY = 27; // Keep above sea level
    if (targetFloorY > Chunk::H - 22) targetFloorY = Chunk::H - 22;

    // Build solid underpinning foundation down to existing ground and clear foliage above
    for (int z = minZ - 1; z <= maxZ + 1; ++z) {
        for (int x = minX - 1; x <= maxX + 1; ++x) {
            int sy = world.surfaceHeight(x, z);
            int startFillY = std::min(sy, targetFloorY);
            for (int y = startFillY - 1; y <= targetFloorY; ++y) {
                safeSet(world, x, y, z, BlockId::Cobblestone);
            }
            // Clear above floor up to height + 14 to clear trees and tall grass
            for (int y = targetFloorY + 1; y <= targetFloorY + 14; ++y) {
                safeSet(world, x, y, z, BlockId::Air);
            }
        }
    }

    return targetFloorY;
}

void VillageGenerator::buildLampPost(World& world, int x, int z) {
    int sy = world.surfaceHeight(x, z);
    if (sy < 26) return;
    safeSet(world, x, sy + 1, z, BlockId::Cobblestone);
    safeSet(world, x, sy + 2, z, BlockId::Wood);
    safeSet(world, x, sy + 3, z, BlockId::Wood);
    safeSet(world, x, sy + 4, z, BlockId::Torch);
    safeSet(world, x, sy + 5, z, BlockId::Air);
}

void VillageGenerator::buildPath(World& world, int x0, int z0, int x1, int z1) {
    int dx = std::abs(x1 - x0);
    int dz = std::abs(z1 - z0);
    int sx = (x0 < x1) ? 1 : -1;
    int sz = (z0 < z1) ? 1 : -1;
    int err = dx - dz;

    int curX = x0;
    int curZ = z0;

    while (true) {
        // Lay 2-wide smooth dirt path with cobblestone stepping stones
        for (int px = -1; px <= 1; ++px) {
            for (int pz = -1; pz <= 1; ++pz) {
                int wx = curX + px;
                int wz = curZ + pz;
                int sy = world.surfaceHeight(wx, wz);
                if (sy >= 26) {
                    BlockId b = safeGet(world, wx, sy, wz);
                    if (b == BlockId::Grass || b == BlockId::Dirt || b == BlockId::TallGrass || b == BlockId::Sand) {
                        const bool isPebble = ((wx * 3 + wz * 7) % 7 == 0);
                        safeSet(world, wx, sy, wz, isPebble ? BlockId::Cobblestone : BlockId::DirtPath);
                        // Clear tall grass above path
                        if (safeGet(world, wx, sy + 1, wz) == BlockId::TallGrass) {
                            safeSet(world, wx, sy + 1, wz, BlockId::Air);
                        }
                    }
                }
            }
        }

        if (curX == x1 && curZ == z1) break;
        int e2 = 2 * err;
        if (e2 > -dz) { err -= dz; curX += sx; }
        if (e2 < dx)  { err += dx; curZ += sz; }
    }
}

void VillageGenerator::buildWell(World& world, int cx, int cz) {
    const int minX = cx - 2;
    const int maxX = cx + 2;
    const int minZ = cz - 2;
    const int maxZ = cz + 2;
    int floorY = prepareFoundation(world, minX, minZ, maxX, maxZ);

    // Rim of cobblestone, water reservoir in middle
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            if (x == minX || x == maxX || z == minZ || z == maxZ) {
                safeSet(world, x, floorY + 1, z, BlockId::Cobblestone);
            } else {
                safeSet(world, x, floorY - 2, z, BlockId::Bedrock);
                safeSet(world, x, floorY - 1, z, BlockId::Water);
                safeSet(world, x, floorY,     z, BlockId::Water);
                safeSet(world, x, floorY + 1, z, BlockId::Air);
            }
        }
    }

    // 4 Corner support pillars (vertical oak logs)
    safeSet(world, minX, floorY + 2, minZ, BlockId::Wood);
    safeSet(world, minX, floorY + 3, minZ, BlockId::Wood);
    safeSet(world, maxX, floorY + 2, minZ, BlockId::Wood);
    safeSet(world, maxX, floorY + 3, minZ, BlockId::Wood);
    safeSet(world, minX, floorY + 2, maxZ, BlockId::Wood);
    safeSet(world, minX, floorY + 3, maxZ, BlockId::Wood);
    safeSet(world, maxX, floorY + 2, maxZ, BlockId::Wood);
    safeSet(world, maxX, floorY + 3, maxZ, BlockId::Wood);

    // Horizontal top frame beams connecting pillars with correct grain orientation
    for (int x = minX + 1; x < maxX; ++x) {
        safeSet(world, x, floorY + 3, minZ, BlockId::WoodX);
        safeSet(world, x, floorY + 3, maxZ, BlockId::WoodX);
    }
    for (int z = minZ + 1; z < maxZ; ++z) {
        safeSet(world, minX, floorY + 3, z, BlockId::WoodZ);
        safeSet(world, maxX, floorY + 3, z, BlockId::WoodZ);
    }

    // Wooden plank roof canopy
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            safeSet(world, x, floorY + 4, z, BlockId::Planks);
        }
    }
    // Center cap & torches
    safeSet(world, cx, floorY + 5, cz, BlockId::Planks);
    safeSet(world, cx, floorY + 6, cz, BlockId::Torch);
}

void VillageGenerator::buildTownHall(World& world, int cx, int cz, int rot) {
    (void)rot;
    const int w = 9;
    const int d = 9;
    const int minX = cx - w / 2;
    const int maxX = minX + w - 1;
    const int minZ = cz - d / 2;
    const int maxZ = minZ + d - 1;
    int floorY = prepareFoundation(world, minX, minZ, maxX, maxZ);

    // Floor
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            safeSet(world, x, floorY, z, BlockId::Planks);
        }
    }

    // Walls, corner wood pillars, and windows
    for (int dy = 1; dy <= 4; ++dy) {
        int y = floorY + dy;
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                bool isCorner = (x == minX || x == maxX) && (z == minZ || z == maxZ);
                bool isWall = (x == minX || x == maxX || z == minZ || z == maxZ);
                if (isCorner) {
                    safeSet(world, x, y, z, BlockId::Wood);
                } else if (isWall) {
                    if (dy == 4) {
                        // Top horizontal framing beam with aligned grain
                        if (z == minZ || z == maxZ) safeSet(world, x, y, z, BlockId::WoodX);
                        else safeSet(world, x, y, z, BlockId::WoodZ);
                    } else if (dy == 2 && ((x == minX + 2 || x == maxX - 2) || (z == minZ + 2 || z == maxZ - 2))) {
                        // Window openings
                        safeSet(world, x, y, z, BlockId::Air);
                    } else if (dy <= 2 && z == maxZ && (x == cx || x == cx + 1)) {
                        // Double doorway entrance at front
                        safeSet(world, x, y, z, BlockId::Air);
                    } else {
                        safeSet(world, x, y, z, (dy == 1) ? BlockId::Cobblestone : BlockId::Planks);
                    }
                } else {
                    safeSet(world, x, y, z, BlockId::Air);
                }
            }
        }
    }

    // Overhanging eave base
    for (int z = minZ - 1; z <= maxZ + 1; ++z) {
        for (int x = minX - 1; x <= maxX + 1; ++x) {
            safeSet(world, x, floorY + 5, z, BlockId::Planks);
        }
    }

    // Sloped wooden roof
    for (int r = 1; r <= 3; ++r) {
        int y = floorY + 5 + r;
        for (int z = minZ - 1 + r; z <= maxZ + 1 - r; ++z) {
            for (int x = minX - 1 + r; x <= maxX + 1 - r; ++x) {
                safeSet(world, x, y, z, BlockId::Planks);
            }
        }
    }

    // Interior meeting table and chandelier lighting
    safeSet(world, cx, floorY + 1, cz, BlockId::Planks);
    safeSet(world, cx + 1, floorY + 1, cz, BlockId::Planks);
    safeSet(world, cx - 1, floorY + 1, cz, BlockId::Planks);

    safeSet(world, minX + 1, floorY + 3, cz, BlockId::Torch);
    safeSet(world, maxX - 1, floorY + 3, cz, BlockId::Torch);
    safeSet(world, cx, floorY + 3, minZ + 1, BlockId::Torch);
    safeSet(world, cx - 1, floorY + 3, maxZ, BlockId::Torch);
    safeSet(world, cx + 2, floorY + 3, maxZ, BlockId::Torch);
}

void VillageGenerator::buildCottage(World& world, int cx, int cz, int width, int depth, int rot) {
    (void)rot;
    const int minX = cx - width / 2;
    const int maxX = minX + width - 1;
    const int minZ = cz - depth / 2;
    const int maxZ = minZ + depth - 1;
    int floorY = prepareFoundation(world, minX, minZ, maxX, maxZ);

    // Floor
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            safeSet(world, x, floorY, z, BlockId::Planks);
        }
    }

    // Walls, corner posts, and doorways
    for (int dy = 1; dy <= 3; ++dy) {
        int y = floorY + dy;
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                bool isCorner = (x == minX || x == maxX) && (z == minZ || z == maxZ);
                bool isWall = (x == minX || x == maxX || z == minZ || z == maxZ);
                if (isCorner) {
                    safeSet(world, x, y, z, BlockId::Wood);
                } else if (isWall) {
                    if (dy == 3) {
                        // Top horizontal wall plate with proper grain
                        if (z == minZ || z == maxZ) safeSet(world, x, y, z, BlockId::WoodX);
                        else safeSet(world, x, y, z, BlockId::WoodZ);
                    } else if (dy <= 2 && z == maxZ && x == cx) {
                        // Doorway at front center
                        safeSet(world, x, y, z, BlockId::Air);
                    } else if (dy == 2 && (x == minX || x == maxX) && z == cz) {
                        // Side window
                        safeSet(world, x, y, z, BlockId::Air);
                    } else {
                        safeSet(world, x, y, z, (dy == 1) ? BlockId::Cobblestone : BlockId::Planks);
                    }
                } else {
                    safeSet(world, x, y, z, BlockId::Air);
                }
            }
        }
    }

    // Eaves overhang (1 block beyond walls)
    for (int z = minZ - 1; z <= maxZ + 1; ++z) {
        for (int x = minX - 1; x <= maxX + 1; ++x) {
            safeSet(world, x, floorY + 4, z, BlockId::Planks);
        }
    }
    // Upper roof tier
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            safeSet(world, x, floorY + 5, z, BlockId::Planks);
        }
    }

    // Interior furnishings: small workbench table and wall torch
    safeSet(world, minX + 1, floorY + 1, minZ + 1, BlockId::Wood);
    safeSet(world, minX + 1, floorY + 2, minZ + 1, BlockId::Torch);
    safeSet(world, maxX - 1, floorY + 2, maxZ - 1, BlockId::Torch);
}

void VillageGenerator::buildWatchtower(World& world, int cx, int cz) {
    const int w = 5;
    const int d = 5;
    const int minX = cx - w / 2;
    const int maxX = minX + w - 1;
    const int minZ = cz - d / 2;
    const int maxZ = minZ + d - 1;
    int floorY = prepareFoundation(world, minX, minZ, maxX, maxZ);

    const int height = 11;

    for (int dy = 0; dy <= height; ++dy) {
        int y = floorY + dy;
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                bool isCorner = (x == minX || x == maxX) && (z == minZ || z == maxZ);
                bool isWall = (x == minX || x == maxX || z == minZ || z == maxZ);

                if (dy == 0 || dy == 5 || dy == 9) {
                    // Floor platforms
                    safeSet(world, x, y, z, BlockId::Cobblestone);
                } else if (dy < 9) {
                    if (isCorner) {
                        safeSet(world, x, y, z, BlockId::Wood);
                    } else if (isWall) {
                        if (dy == 4 || dy == 8) {
                            // Support horizontal timber ring
                            if (z == minZ || z == maxZ) safeSet(world, x, y, z, BlockId::WoodX);
                            else safeSet(world, x, y, z, BlockId::WoodZ);
                        } else if (dy <= 2 && z == maxZ && x == cx) {
                            // Ground entrance
                            safeSet(world, x, y, z, BlockId::Air);
                        } else if (dy == 3 || dy == 7) {
                            // Arrow slits
                            safeSet(world, x, y, z, BlockId::Air);
                        } else {
                            safeSet(world, x, y, z, BlockId::Cobblestone);
                        }
                    } else {
                        safeSet(world, x, y, z, BlockId::Air);
                    }
                } else if (dy == 10) {
                    // Battlements
                    if (isCorner || (x == cx && (z == minZ || z == maxZ)) || (z == cz && (x == minX || x == maxX))) {
                        safeSet(world, x, y, z, BlockId::Cobblestone);
                    } else {
                        safeSet(world, x, y, z, BlockId::Air);
                    }
                }
            }
        }
    }

    // Top deck corner torches
    safeSet(world, minX, floorY + 11, minZ, BlockId::Torch);
    safeSet(world, maxX, floorY + 11, minZ, BlockId::Torch);
    safeSet(world, minX, floorY + 11, maxZ, BlockId::Torch);
    safeSet(world, maxX, floorY + 11, maxZ, BlockId::Torch);
}

void VillageGenerator::buildFarmPlot(World& world, int cx, int cz, int width, int depth) {
    const int minX = cx - width / 2;
    const int maxX = minX + width - 1;
    const int minZ = cz - depth / 2;
    const int maxZ = minZ + depth - 1;
    int floorY = prepareFoundation(world, minX, minZ, maxX, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            bool isBorder = (x == minX || x == maxX || z == minZ || z == maxZ);
            if (isBorder) {
                if (z == minZ || z == maxZ) safeSet(world, x, floorY, z, BlockId::WoodX);
                else safeSet(world, x, floorY, z, BlockId::WoodZ);
            } else if (x == cx) {
                // Central irrigated water canal
                safeSet(world, x, floorY, z, BlockId::Water);
            } else {
                // Cultivated crops on soil
                safeSet(world, x, floorY, z, BlockId::Dirt);
                if (hashCoord(x, z, 77) % 3 != 0) {
                    safeSet(world, x, floorY + 1, z, BlockId::TallGrass);
                }
            }
        }
    }
    // Corner light posts
    buildLampPost(world, minX, minZ);
    buildLampPost(world, maxX, maxZ);
}

void VillageGenerator::buildVillage(World& world, int centerX, int centerZ, int templateType, uint32_t vSeed) {
    (void)vSeed;
    // 1. Central Village Well / Plaza
    buildWell(world, centerX, centerZ);

    switch (templateType % 4) {
        case 0: {
            // === TEMPLATE 0: Crossroad Settlement ===
            // North: Town Hall
            buildTownHall(world, centerX, centerZ - 18);
            buildPath(world, centerX, centerZ - 3, centerX, centerZ - 13);

            // South: Large Cottage
            buildCottage(world, centerX, centerZ + 16, 7, 7);
            buildPath(world, centerX, centerZ + 3, centerX, centerZ + 12);

            // East: Farm Plot
            buildFarmPlot(world, centerX + 18, centerZ, 7, 7);
            buildPath(world, centerX + 3, centerZ, centerX + 14, centerZ);

            // West: Small Cottage
            buildCottage(world, centerX - 16, centerZ, 6, 6);
            buildPath(world, centerX - 3, centerZ, centerX - 13, centerZ);

            // North-East: Watchtower
            buildWatchtower(world, centerX + 15, centerZ - 16);
            buildPath(world, centerX + 9, centerZ - 9, centerX + 13, centerZ - 14);

            // Street Lamps
            buildLampPost(world, centerX - 6, centerZ - 6);
            buildLampPost(world, centerX + 6, centerZ + 6);
            break;
        }
        case 1: {
            // === TEMPLATE 1: Valley High Street ===
            // Linear main road
            buildPath(world, centerX - 25, centerZ, centerX + 25, centerZ);

            // North side: Town Hall & Cottage
            buildTownHall(world, centerX - 12, centerZ - 15);
            buildCottage(world, centerX + 12, centerZ - 14, 6, 6);

            // South side: Farm, Smithy Cottage, Watchtower
            buildFarmPlot(world, centerX - 14, centerZ + 14, 7, 7);
            buildCottage(world, centerX + 6, centerZ + 13, 6, 5);
            buildWatchtower(world, centerX + 22, centerZ - 12);

            // Streetlamps along road
            buildLampPost(world, centerX - 18, centerZ + 3);
            buildLampPost(world, centerX, centerZ - 3);
            buildLampPost(world, centerX + 18, centerZ + 3);
            break;
        }
        case 2: {
            // === TEMPLATE 2: Ring Commons / Market Square ===
            // Radial paths
            buildPath(world, centerX, centerZ, centerX - 16, centerZ - 14);
            buildPath(world, centerX, centerZ, centerX + 16, centerZ - 14);
            buildPath(world, centerX, centerZ, centerX - 16, centerZ + 14);
            buildPath(world, centerX, centerZ, centerX + 16, centerZ + 14);

            // 3 Surrounding Cottages & 1 Town Hall
            buildTownHall(world, centerX, centerZ - 20);
            buildCottage(world, centerX - 18, centerZ - 10, 6, 6);
            buildCottage(world, centerX + 18, centerZ - 10, 6, 6);
            buildCottage(world, centerX - 16, centerZ + 16, 6, 5);

            // Farm & Watchtower
            buildFarmPlot(world, centerX + 16, centerZ + 16, 7, 7);
            buildWatchtower(world, centerX, centerZ + 22);

            buildLampPost(world, centerX - 8, centerZ);
            buildLampPost(world, centerX + 8, centerZ);
            break;
        }
        case 3:
        default: {
            // === TEMPLATE 3: Fortified Enclave ===
            buildTownHall(world, centerX, centerZ - 14);
            buildPath(world, centerX, centerZ - 3, centerX, centerZ - 9);

            // Two Watchtowers flanking front
            buildWatchtower(world, centerX - 18, centerZ + 16);
            buildWatchtower(world, centerX + 18, centerZ + 16);
            buildPath(world, centerX - 14, centerZ + 14, centerX + 14, centerZ + 14);

            // Side cottages
            buildCottage(world, centerX - 18, centerZ - 4, 6, 6);
            buildCottage(world, centerX + 18, centerZ - 4, 6, 6);
            buildFarmPlot(world, centerX, centerZ + 18, 7, 5);

            buildLampPost(world, centerX - 10, centerZ + 5);
            buildLampPost(world, centerX + 10, centerZ + 5);
            break;
        }
    }
}

void VillageGenerator::locateVillages(const World& world, uint32_t seed, std::vector<Village>& outVillages) {
    outVillages.clear();

    int bestX = 0;
    int bestZ = 0;
    int bestElevation = 0;
    bool foundSpot = false;

    // Search outward from spawn for a wide, flat, dry land area
    for (int r = 16; r <= 96 && !foundSpot; r += 8) {
        for (int dz = -r; dz <= r && !foundSpot; dz += 8) {
            for (int dx = -r; dx <= r && !foundSpot; dx += 8) {
                if (std::abs(dx) != r && std::abs(dz) != r) continue;

                const int testX = dx;
                const int testZ = dz;

                // Test entire footprint (radius 18) for water/ocean avoidance
                bool validDryArea = true;
                int sumY = 0;
                int samples = 0;
                int minY = 999;
                int maxY = -999;

                for (int fz = -18; fz <= 18 && validDryArea; fz += 6) {
                    for (int fx = -18; fx <= 18 && validDryArea; fx += 6) {
                        const int sx = testX + fx;
                        const int sz = testZ + fz;
                        const int sy = world.surfaceHeight(sx, sz);
                        const BlockId b = safeGet(world, sx, sy, sz);

                        // Strict rejection of ocean, water, sandy coastlines, or steep cliffs
                        if (sy < 28 || sy > 52 || b == BlockId::Water || b == BlockId::Sand) {
                            validDryArea = false;
                            break;
                        }
                        if (sy < minY) minY = sy;
                        if (sy > maxY) maxY = sy;
                        sumY += sy;
                        samples++;
                    }
                }

                // If height variance is gentle and zero water in village footprint
                if (validDryArea && samples > 0 && (maxY - minY <= 6)) {
                    bestX = testX;
                    bestZ = testZ;
                    bestElevation = sumY / samples;
                    foundSpot = true;
                    break;
                }
            }
        }
    }

    // Only place village if a valid dry land area was located
    if (foundSpot) {
        const int templateType = static_cast<int>(seed % 4);

        Village v;
        v.center = glm::vec3(static_cast<float>(bestX) + 0.5f,
                             static_cast<float>(bestElevation + 1),
                             static_cast<float>(bestZ) + 0.5f);
        v.radius = 36;
        v.templateType = templateType;
        v.name = "Pigman Village";
        outVillages.push_back(v);
    }
}

void VillageGenerator::generateVillages(World& world, uint32_t seed, std::vector<Village>& outVillages) {
    locateVillages(world, seed, outVillages);

    for (size_t i = 0; i < outVillages.size(); ++i) {
        const auto& v = outVillages[i];
        const int cx = static_cast<int>(std::floor(v.center.x));
        const int cz = static_cast<int>(std::floor(v.center.z));
        const uint32_t vSeed = (outVillages.size() == 1) ? seed : (seed + static_cast<uint32_t>(i) * 997);
        buildVillage(world, cx, cz, v.templateType, vSeed);
    }
}

} // namespace vox
