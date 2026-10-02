#include "world/BackroomsGenerator.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/FastNoiseLite.hpp"
#include "world/World.hpp"

#include <cmath>
#include <cstdlib>

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

float rndFloat(int x, int z, uint32_t seed) {
    return static_cast<float>(hashCoord(x, z, seed) & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

} // namespace

OchreArchetype BackroomsGenerator::getArchetype(int cx, int cz, uint32_t seed) {
    const uint32_t h = hashCoord(cx, cz, seed) % 100;
    if (h < 55) return OchreArchetype::StandardCorridors; // 55%
    if (h < 70) return OchreArchetype::FilingArchives;    // 15%
    if (h < 80) return OchreArchetype::RedstoneConduits;  // 10%
    if (h < 90) return OchreArchetype::DrainageSump;      // 10%
    return OchreArchetype::FalseOverworld;                // 10%
}

bool BackroomsGenerator::isBlackoutZone(int wx, int wz, uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 999));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFrequency(0.025f);
    return fnl.GetNoise(static_cast<float>(wx), static_cast<float>(wz)) < -0.45f;
}

bool BackroomsGenerator::isPillar(int wx, int wz, uint32_t seed) {
    // 2x2 Support pillars in open hall areas
    const int modX = (wx % 16 + 16) % 16;
    const int modZ = (wz % 16 + 16) % 16;
    const int cellX = (wx >= 0 ? wx : wx - 15) / 16;
    const int cellZ = (wz >= 0 ? wz : wz - 15) / 16;

    const float pillarRoll = rndFloat(cellX, cellZ, seed + 101);
    if (pillarRoll > 0.45f) {
        if ((modX == 7 || modX == 8) && (modZ == 7 || modZ == 8)) {
            return true;
        }
    }
    return false;
}

bool BackroomsGenerator::isWall(int wx, int wz, uint32_t seed) {
    if (isPillar(wx, wz, seed)) return true;

    // 2-block thick grid cellular maze partitions
    const int cellDim = 14;
    const int gx = (wx % cellDim + cellDim) % cellDim;
    const int gz = (wz % cellDim + cellDim) % cellDim;
    const int cx = (wx >= 0 ? wx : wx - (cellDim - 1)) / cellDim;
    const int cz = (wz >= 0 ? wz : wz - (cellDim - 1)) / cellDim;

    // Macro room type
    FastNoiseLite fnlRoom(static_cast<int>(seed + 555));
    fnlRoom.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    fnlRoom.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Manhattan);
    fnlRoom.SetFrequency(0.035f);
    const float roomType = fnlRoom.GetNoise(static_cast<float>(wx), static_cast<float>(wz));

    if (roomType > 0.40f) {
        // Open Column Hall with sparse 2-block perimeter walls
        if ((gx == 0 || gx == 1) && (gz < 4 || gz > 9)) return true;
        if ((gz == 0 || gz == 1) && (gx < 4 || gx > 9)) return true;
        return false;
    }

    // 2-block thick partition walls with 3-wide doorways
    const float wallRollX = rndFloat(cx, cz, seed + 303);
    const float wallRollZ = rndFloat(cx, cz, seed + 404);

    if (wallRollX > 0.35f && (gx == 0 || gx == 1)) {
        // Doorway opening at center (gz = 5, 6, 7)
        if (gz < 5 || gz > 7) return true;
    }

    if (wallRollZ > 0.35f && (gz == 0 || gz == 1)) {
        // Doorway opening at center (gx = 5, 6, 7)
        if (gx < 5 || gx > 7) return true;
    }

    // Interior divider partitions
    if (wallRollX > 0.65f && (gx == 7 || gx == 8) && gz >= 3 && gz <= 10) return true;
    if (wallRollZ > 0.65f && (gz == 7 || gz == 8) && gx >= 3 && gx <= 10) return true;

    return false;
}

bool BackroomsGenerator::isInnerCore(int wx, int wz, uint32_t seed) {
    // 1-block inner core if all 4 orthogonal neighbours are walls
    return isWall(wx + 1, wz, seed) &&
           isWall(wx - 1, wz, seed) &&
           isWall(wx, wz + 1, seed) &&
           isWall(wx, wz - 1, seed);
}

bool BackroomsGenerator::isLight(int wx, int wz, uint32_t seed) {
    if (isBlackoutZone(wx, wz, seed)) return false;

    // Resonant Lanterns spaced regularly on ceiling
    const int modX = (wx % 4 + 4) % 4;
    const int modZ = (wz % 4 + 4) % 4;

    if ((modX == 1 && modZ == 1) || (modX == 3 && modZ == 3)) {
        return rndFloat(wx, wz, seed + 777) > 0.08f;
    }
    return false;
}

bool BackroomsGenerator::isExitDoor(int wx, int wz, uint32_t seed) {
    const int cellDim = 160;
    const int cellX = (wx >= 0 ? wx : wx - (cellDim - 1)) / cellDim;
    const int cellZ = (wz >= 0 ? wz : wz - (cellDim - 1)) / cellDim;
    const uint32_t h = hashCoord(cellX, cellZ, seed + 9999);
    if ((h & 0xFF) > 128) return false;
    const int targetX = cellX * cellDim + static_cast<int>((h >> 8) % (cellDim - 16)) + 8;
    const int targetZ = cellZ * cellDim + static_cast<int>((h >> 16) % (cellDim - 16)) + 8;
    return wx == targetX && wz == targetZ;
}

bool BackroomsGenerator::isAlmondWater(int wx, int wz, uint32_t seed) {
    const int cellX = (wx >= 0 ? wx : wx - 31) / 32;
    const int cellZ = (wz >= 0 ? wz : wz - 31) / 32;
    const uint32_t h = hashCoord(cellX, cellZ, seed + 8888);
    const int targetX = cellX * 32 + static_cast<int>(h % 26) + 3;
    const int targetZ = cellZ * 32 + static_cast<int>((h >> 16) % 26) + 3;
    return wx == targetX && wz == targetZ;
}

bool BackroomsGenerator::isGlitch(int wx, int wz, uint32_t seed) {
    const int cellDim = 128;
    const int cellX = (wx >= 0 ? wx : wx - (cellDim - 1)) / cellDim;
    const int cellZ = (wz >= 0 ? wz : wz - (cellDim - 1)) / cellDim;
    const uint32_t h = hashCoord(cellX, cellZ, seed + 7777);
    if ((h & 0xFF) > 160) return false;
    const int targetX = cellX * cellDim + static_cast<int>((h >> 8) % (cellDim - 16)) + 8;
    const int targetZ = cellZ * cellDim + static_cast<int>((h >> 16) % (cellDim - 16)) + 8;
    return wx == targetX && wz == targetZ;
}

void BackroomsGenerator::generateChunk(World& world, Chunk& chunk) const {
    (void)world;
    const int ox = chunk.originX();
    const int oz = chunk.originZ();
    const int cx = (ox >= 0 ? ox : ox - 15) / 16;
    const int cz = (oz >= 0 ? oz : oz - 15) / 16;

    const OchreArchetype arch = getArchetype(cx, cz, m_seed);

    for (int lz = 0; lz < Chunk::D; ++lz) {
        const int wz = oz + lz;
        for (int lx = 0; lx < Chunk::W; ++lx) {
            const int wx = ox + lx;

            // 1. Bedrock base foundation (Y = 0)
            chunk.set(lx, 0, lz, BlockId::Bedrock);

            if (arch == OchreArchetype::RedstoneConduits) {
                // --- REDSTONE CONDUITS ARCHETYPE ---
                // Crouching 2-block headroom: ceiling at Y=3, Bedrock seal Y>=4
                chunk.set(lx, 1, lz, ((lx + lz) % 4 == 0) ? BlockId::Cobblestone : BlockId::DampOchreWool);

                const bool wall = isWall(wx, wz, m_seed);
                if (wall) {
                    chunk.set(lx, 2, lz, BlockId::Cobblestone);
                } else {
                    chunk.set(lx, 2, lz, ((wx + wz) % 7 == 0) ? BlockId::IronOre : BlockId::Air);
                }

                // Ceiling (Y = 3)
                chunk.set(lx, 3, lz, BlockId::Cobblestone);

                // Bedrock seal (Y >= 4)
                for (int y = 4; y < Chunk::H; ++y) {
                    chunk.set(lx, y, lz, BlockId::Bedrock);
                }
                continue;
            }

            if (arch == OchreArchetype::DrainageSump) {
                // --- DRAINAGE SUMP ARCHETYPE ---
                // Sunken floor with Y=1 water basin
                chunk.set(lx, 1, lz, BlockId::Water);

                const bool wall = isWall(wx, wz, m_seed);
                for (int y = 2; y <= 4; ++y) {
                    if (wall) {
                        chunk.set(lx, y, lz, (y == 2) ? BlockId::Cobblestone : BlockId::OchrePlaster);
                    } else {
                        chunk.set(lx, y, lz, BlockId::Air);
                    }
                }

                // Leaking ceiling
                const bool leak = ((wx % 8 == 0) && (wz % 8 == 0));
                chunk.set(lx, 5, lz, leak ? BlockId::Air : BlockId::ChiseledLimestone);

                for (int y = 6; y < Chunk::H; ++y) {
                    chunk.set(lx, y, lz, BlockId::Bedrock);
                }
                continue;
            }

            if (arch == OchreArchetype::FalseOverworld) {
                // --- FALSE OVERWORLD ROOM ARCHETYPE ---
                // Grass / dirt floor with artificial leafless oak trunk in center
                const bool isCenter = (lx >= 6 && lx <= 9 && lz >= 6 && lz <= 9);
                chunk.set(lx, 1, lz, isCenter ? BlockId::Grass : BlockId::Dirt);

                const bool wall = isWall(wx, wz, m_seed);
                const bool isTrunk = (lx == 7 && lz == 7);

                for (int y = 2; y <= 4; ++y) {
                    if (isTrunk) {
                        chunk.set(lx, y, lz, BlockId::Wood);
                    } else if (wall) {
                        chunk.set(lx, y, lz, (y == 2) ? BlockId::OchrePlasterBase : BlockId::OchrePlaster);
                    } else {
                        if (y == 2 && ((lx * 3 + lz * 7) % 5 == 0)) {
                            chunk.set(lx, 2, lz, BlockId::TallGrass);
                        } else {
                            chunk.set(lx, y, lz, BlockId::Air);
                        }
                    }
                }

                // Ceiling with center Resonant Lantern mimicking sun
                if (isCenter && isLight(wx, wz, m_seed)) {
                    chunk.set(lx, 5, lz, BlockId::ResonantLantern);
                } else {
                    chunk.set(lx, 5, lz, BlockId::ChiseledLimestone);
                }

                for (int y = 6; y < Chunk::H; ++y) {
                    chunk.set(lx, y, lz, BlockId::Bedrock);
                }
                continue;
            }

            if (arch == OchreArchetype::FilingArchives) {
                // --- THE FILING ARCHIVES ARCHETYPE ---
                chunk.set(lx, 1, lz, BlockId::DampOchreWool);

                const bool isAisle = (lx % 3 == 0) && (lz < 5 || lz > 10);
                const bool wall = isWall(wx, wz, m_seed);

                for (int y = 2; y <= 4; ++y) {
                    if (wall) {
                        chunk.set(lx, y, lz, (y == 2) ? BlockId::OchrePlasterBase : BlockId::OchrePlaster);
                    } else if (isAisle) {
                        chunk.set(lx, y, lz, (y == 4) ? BlockId::CraftingTable : BlockId::Planks);
                    } else {
                        if (y == 2 && lx == 8 && lz == 8) {
                            chunk.set(lx, 2, lz, BlockId::CondensationFlask);
                        } else {
                            chunk.set(lx, y, lz, BlockId::Air);
                        }
                    }
                }

                if (isLight(wx, wz, m_seed) && !wall && !isAisle) {
                    chunk.set(lx, 5, lz, BlockId::ResonantLantern);
                } else {
                    chunk.set(lx, 5, lz, BlockId::ChiseledLimestone);
                }

                for (int y = 6; y < Chunk::H; ++y) {
                    chunk.set(lx, y, lz, BlockId::Bedrock);
                }
                continue;
            }

            // --- STANDARD CORRIDORS (55%) ---
            // 2. Floor: Damp Ochre Wool
            chunk.set(lx, 1, lz, BlockId::DampOchreWool);

            // 3. 2-block thick walls with outer OchrePlaster and inner Wood framing / cavity
            const bool wall = isWall(wx, wz, m_seed);
            const bool innerCore = wall && isInnerCore(wx, wz, m_seed);
            const bool exitDoor = isExitDoor(wx, wz, m_seed);
            const bool glitch = isGlitch(wx, wz, m_seed);
            const bool almondWater = isAlmondWater(wx, wz, m_seed);

            for (int y = 2; y <= 4; ++y) {
                if (exitDoor && y == 2) {
                    chunk.set(lx, y, lz, BlockId::VaultHatch);
                } else if (exitDoor && (y == 3 || y == 4)) {
                    chunk.set(lx, y, lz, BlockId::Air);
                } else if (glitch && y == 2 && wall) {
                    chunk.set(lx, y, lz, BlockId::FracturedBedrock);
                } else if (almondWater && y == 2 && !wall) {
                    chunk.set(lx, y, lz, BlockId::CondensationFlask);
                } else if (innerCore) {
                    // Procedural Wood stud framing or hollow cavity
                    const bool isPost = ((wx + wz) % 2 == 0);
                    chunk.set(lx, y, lz, isPost ? BlockId::Wood : BlockId::Air);
                } else if (wall) {
                    chunk.set(lx, y, lz, (y == 2) ? BlockId::OchrePlasterBase : BlockId::OchrePlaster);
                } else {
                    chunk.set(lx, y, lz, BlockId::Air);
                }
            }

            // 4. Ceiling: Chiseled Limestone & Resonant Lanterns (Y = 5)
            if (isLight(wx, wz, m_seed) && !wall) {
                chunk.set(lx, 5, lz, BlockId::ResonantLantern);
            } else {
                chunk.set(lx, 5, lz, BlockId::ChiseledLimestone);
            }

            // 5. Solid ceiling bedrock barrier sealing the chasm (Y >= 6)
            for (int y = 6; y < Chunk::H; ++y) {
                chunk.set(lx, y, lz, BlockId::Bedrock);
            }
        }
    }
}

glm::vec3 BackroomsGenerator::findSafeSpawn(World& world, int startX, int startZ) const {
    for (int r = 0; r <= 32; ++r) {
        for (int dz = -r; dz <= r; ++dz) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dz)) != r) continue;
                const int wx = startX + dx;
                const int wz = startZ + dz;

                const BlockId bFloor = world.getBlock(wx, 1, wz);
                const BlockId bFeet = world.getBlock(wx, 2, wz);
                const BlockId bHead = world.getBlock(wx, 3, wz);

                if ((bFloor == BlockId::DampOchreWool || bFloor == BlockId::Grass) && isAir(bFeet) && isAir(bHead)) {
                    const BlockId n1 = world.getBlock(wx + 1, 2, wz);
                    const BlockId n2 = world.getBlock(wx - 1, 2, wz);
                    const BlockId n3 = world.getBlock(wx, 2, wz + 1);
                    const BlockId n4 = world.getBlock(wx, 2, wz - 1);
                    int openCount = 0;
                    if (isAir(n1)) openCount++;
                    if (isAir(n2)) openCount++;
                    if (isAir(n3)) openCount++;
                    if (isAir(n4)) openCount++;

                    if (openCount >= 2) {
                        return glm::vec3(static_cast<float>(wx) + 0.5f, 2.0f, static_cast<float>(wz) + 0.5f);
                    }
                }
            }
        }
    }
    return glm::vec3(static_cast<float>(startX) + 0.5f, 2.0f, static_cast<float>(startZ) + 0.5f);
}

} // namespace vox
