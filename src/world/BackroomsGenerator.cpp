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

bool BackroomsGenerator::isBlackoutZone(int wx, int wz, uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 999));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFrequency(0.025f);
    return fnl.GetNoise(static_cast<float>(wx), static_cast<float>(wz)) < -0.45f;
}

bool BackroomsGenerator::isPillar(int wx, int wz, uint32_t seed) {
    // 2x2 Support pillars in open office hall areas
    const int modX = (wx % 16 + 16) % 16;
    const int modZ = (wz % 16 + 16) % 16;
    const int cellX = wx / 16;
    const int cellZ = wz / 16;

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

    // Grid cellular maze partitions (Level 0 layout)
    const int cellDim = 12;
    const int gx = (wx % cellDim + cellDim) % cellDim;
    const int gz = (wz % cellDim + cellDim) % cellDim;
    const int cx = wx / cellDim;
    const int cz = wz / cellDim;

    // Macro room type: open hall, partitioned cubicles, or winding corridors
    FastNoiseLite fnlRoom(static_cast<int>(seed + 555));
    fnlRoom.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    fnlRoom.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Manhattan);
    fnlRoom.SetFrequency(0.035f);
    const float roomType = fnlRoom.GetNoise(static_cast<float>(wx), static_cast<float>(wz));

    if (roomType > 0.40f) {
        // Open Column Hall with sparse perimeter walls
        if (gx == 0 && (gz < 4 || gz > 7)) return true;
        if (gz == 0 && (gx < 4 || gx > 7)) return true;
        return false;
    }

    // Partitioned yellow maze walls with doorways
    const float wallRollX = rndFloat(cx, cz, seed + 303);
    const float wallRollZ = rndFloat(cx, cz, seed + 404);

    if (wallRollX > 0.35f && gx == 0) {
        // Doorway opening at center (gz = 5, 6)
        if (gz != 5 && gz != 6) return true;
    }

    if (wallRollZ > 0.35f && gz == 0) {
        // Doorway opening at center (gx = 5, 6)
        if (gx != 5 && gx != 6) return true;
    }

    // Interior cubicle partition dividers
    if (wallRollX > 0.65f && gx == 6 && gz >= 3 && gz <= 9) return true;
    if (wallRollZ > 0.65f && gz == 6 && gx >= 3 && gx <= 9) return true;

    return false;
}

bool BackroomsGenerator::isLight(int wx, int wz, uint32_t seed) {
    if (isBlackoutZone(wx, wz, seed)) return false;

    // Fluorescent ceiling fixtures arranged regularly in bright office rooms
    const int modX = (wx % 4 + 4) % 4;
    const int modZ = (wz % 4 + 4) % 4;

    if ((modX == 1 && modZ == 1) || (modX == 3 && modZ == 3)) {
        // 94% working light, 6% individual flickering dead bulbs
        return rndFloat(wx, wz, seed + 777) > 0.06f;
    }
    return false;
}

bool BackroomsGenerator::isExitDoor(int wx, int wz, uint32_t seed) {
    // Rare Fire Exit Door placed in deep maze corridors (~1 per 160x160 area with 50% spawn chance)
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

    for (int lz = 0; lz < Chunk::D; ++lz) {
        const int wz = oz + lz;
        for (int lx = 0; lx < Chunk::W; ++lx) {
            const int wx = ox + lx;

            // 1. Bedrock base floor
            chunk.set(lx, 0, lz, BlockId::Bedrock);

            // 2. Floor pit check (rare square holes into subfloor)
            const int modPitX = (wx % 64 + 64) % 64;
            const int modPitZ = (wz % 64 + 64) % 64;
            const bool isPit = (modPitX >= 30 && modPitX <= 31 && modPitZ >= 30 && modPitZ <= 31);

            if (isPit) {
                chunk.set(lx, 1, lz, BlockId::Air);
            } else {
                chunk.set(lx, 1, lz, BlockId::BackroomsCarpet);
            }

            // 3. Walls & Hallway air volume (Y=2..4)
            const bool wall = isWall(wx, wz, m_seed);
            const bool exitDoor = isExitDoor(wx, wz, m_seed);
            const bool glitch = isGlitch(wx, wz, m_seed);
            const bool almondWater = isAlmondWater(wx, wz, m_seed);

            for (int y = 2; y <= 4; ++y) {
                if (exitDoor && y == 2) {
                    chunk.set(lx, y, lz, BlockId::ExitDoor);
                } else if (exitDoor && y == 3) {
                    chunk.set(lx, y, lz, BlockId::Air);
                } else if (glitch && y == 2 && wall) {
                    chunk.set(lx, y, lz, BlockId::GlitchBlock);
                } else if (almondWater && y == 2 && !wall) {
                    chunk.set(lx, y, lz, BlockId::AlmondWater);
                } else if (wall) {
                    chunk.set(lx, y, lz, (y == 2) ? BlockId::BackroomsWallpaperBase : BlockId::BackroomsWallpaper);
                } else {
                    chunk.set(lx, y, lz, BlockId::Air);
                }
            }

            // 4. Acoustic Ceiling Tile & Fluorescent Lights (Y=5)
            if (isLight(wx, wz, m_seed) && !wall) {
                chunk.set(lx, 5, lz, BlockId::FluorescentLight);
            } else {
                chunk.set(lx, 5, lz, BlockId::BackroomsCeiling);
            }

            // 5. Solid ceiling bedrock barrier (Y=6)
            chunk.set(lx, 6, lz, BlockId::Bedrock);

            // Empty above ceiling (Y=7..H-1)
            for (int y = 7; y < Chunk::H; ++y) {
                chunk.set(lx, y, lz, BlockId::Air);
            }
        }
    }
}

glm::vec3 BackroomsGenerator::findSafeSpawn(World& world, int startX, int startZ) const {
    // Spiral outward from (startX, startZ) to locate open carpet hallway with no wall obstructions
    for (int r = 0; r <= 32; ++r) {
        for (int dz = -r; dz <= r; ++dz) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dz)) != r) continue;
                const int wx = startX + dx;
                const int wz = startZ + dz;

                const BlockId bFloor = world.getBlock(wx, 1, wz);
                const BlockId bFeet = world.getBlock(wx, 2, wz);
                const BlockId bHead = world.getBlock(wx, 3, wz);

                if (bFloor == BlockId::BackroomsCarpet && isAir(bFeet) && isAir(bHead)) {
                    // Check orthogonal clearance so player doesn't spawn pressed against a wall
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
