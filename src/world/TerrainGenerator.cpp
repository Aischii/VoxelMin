#include "world/TerrainGenerator.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/FastNoiseLite.hpp"
#include "world/VillageGenerator.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vox {

namespace {

FastNoiseLite makeContinentalnessNoise(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 101));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(4);
    fnl.SetFractalLacunarity(2.0f);
    fnl.SetFractalGain(0.5f);
    fnl.SetFrequency(0.0030f);
    return fnl;
}

FastNoiseLite makeErosionNoise(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 202));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(3);
    fnl.SetFractalLacunarity(2.0f);
    fnl.SetFractalGain(0.5f);
    fnl.SetFrequency(0.0070f);
    return fnl;
}

FastNoiseLite makePeaksNoise(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 777));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fnl.SetFractalType(FastNoiseLite::FractalType_Ridged);
    fnl.SetFractalOctaves(3);
    fnl.SetFractalLacunarity(2.0f);
    fnl.SetFractalGain(0.55f);
    fnl.SetFrequency(0.012f);
    return fnl;
}

FastNoiseLite makeDetailNoise(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 303));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(3);
    fnl.SetFractalLacunarity(2.0f);
    fnl.SetFractalGain(0.5f);
    fnl.SetFrequency(0.024f);
    return fnl;
}

FastNoiseLite makeRiverNoise(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 404));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fnl.SetFrequency(0.0035f);
    return fnl;
}

FastNoiseLite makeCaveNoise1(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 501));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(2);
    fnl.SetFrequency(0.045f);
    return fnl;
}

FastNoiseLite makeCaveNoise2(uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed + 607));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(2);
    fnl.SetFrequency(0.045f);
    return fnl;
}

} // namespace

float TerrainGenerator::hash2(int x, int z, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393U +
                 static_cast<uint32_t>(z) * 668265263U +
                 seed * 362437U;
    h = (h ^ (h >> 13)) * 1274126177U;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

float TerrainGenerator::valueNoise(float x, float z, uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFrequency(0.02f);
    return (fnl.GetNoise(x, z) + 1.0f) * 0.5f;
}

float TerrainGenerator::fbm(float x, float z, uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl.SetFractalOctaves(4);
    fnl.SetFrequency(0.02f);
    return (fnl.GetNoise(x, z) + 1.0f) * 0.5f;
}

float TerrainGenerator::hash3(int x, int y, int z, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393U +
                 static_cast<uint32_t>(y) * 668265263U +
                 static_cast<uint32_t>(z) * 1274126177U +
                 seed * 362437U;
    h = (h ^ (h >> 13)) * 1274126177U;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

float TerrainGenerator::noise3D(float x, float y, float z, uint32_t seed) {
    FastNoiseLite fnl(static_cast<int>(seed));
    fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fnl.SetFrequency(0.05f);
    return (fnl.GetNoise(x, y, z) + 1.0f) * 0.5f;
}

float TerrainGenerator::sampleContinentalness(float wx, float wz, uint32_t seed) {
    FastNoiseLite fnl = makeContinentalnessNoise(seed);
    return (fnl.GetNoise(wx, wz) + 1.0f) * 0.5f;
}

float TerrainGenerator::sampleRiver(float wx, float wz, uint32_t seed) {
    FastNoiseLite fnl = makeRiverNoise(seed);
    const float r1 = (fnl.GetNoise(wx, wz) + 1.0f) * 0.5f;
    const float r2 = (fnl.GetNoise(wx + 89.0f, wz + 89.0f) + 1.0f) * 0.5f;
    return std::abs(r1 - r2);
}

float TerrainGenerator::sampleTerrainHeight(float wx, float wz, uint32_t seed, WorldType type) {
    if (type == WorldType::Flat) {
        return 4.0f;
    }

    FastNoiseLite fnlCont = makeContinentalnessNoise(seed);
    FastNoiseLite fnlErosion = makeErosionNoise(seed);
    FastNoiseLite fnlPeaks = makePeaksNoise(seed);
    FastNoiseLite fnlDetail = makeDetailNoise(seed);

    if (type == WorldType::Island) {
        const float n = (fnlDetail.GetNoise(wx, wz) + 1.0f) * 0.5f;
        float rawH = 32.0f + n * 18.0f;
        const float dist = std::sqrt(wx * wx + wz * wz) / 240.0f;
        const float mask = std::clamp((1.0f - dist) * 1.5f, 0.0f, 1.0f);
        return std::clamp(14.0f + (rawH - 14.0f) * mask, 1.0f, static_cast<float>(Chunk::H - 4));
    }

    if (type == WorldType::Mountainous) {
        const float n = (fnlCont.GetNoise(wx, wz) + 1.0f) * 0.5f;
        const float ridge = (fnlPeaks.GetNoise(wx, wz) + 1.0f) * 0.5f;
        return std::clamp(28.0f + n * 22.0f + ridge * ridge * 26.0f, 1.0f, static_cast<float>(Chunk::H - 4));
    }

    if (type == WorldType::Cavernous) {
        const float n = (fnlDetail.GetNoise(wx, wz) + 1.0f) * 0.5f;
        return std::clamp(26.0f + n * 18.0f, 1.0f, static_cast<float>(Chunk::H - 4));
    }

    // --- WorldType::Default: Multi-Biome Continuous World Generation (Deep & High) ---
    const float cont = (fnlCont.GetNoise(wx, wz) + 1.0f) * 0.5f;
    const float erosion = (fnlErosion.GetNoise(wx, wz) + 1.0f) * 0.5f;
    const float detail = (fnlDetail.GetNoise(wx, wz) + 1.0f) * 0.5f;

    float rawH = 26.0f;

    if (cont < 0.30f) {
        // 1. Lake & Deep Ocean Basins (Deep depressions down to Y=10 below sea level 26)
        const float depth = (0.30f - cont) / 0.30f;
        rawH = 10.0f + (1.0f - depth) * 15.0f + (detail - 0.5f) * 4.0f;
    } else if (cont < 0.38f) {
        // 2. Coastal Shoreline & Sandy Beaches (Y=25-28)
        const float t = (cont - 0.30f) / 0.08f;
        rawH = 25.0f + t * 3.0f + (detail - 0.5f) * 2.0f;
    } else if (cont < 0.62f) {
        // 3. Lowland Plains & Forests (Rolling meadows and hills, Y=28-37)
        const float t = (cont - 0.38f) / 0.24f;
        rawH = 28.0f + t * 9.0f + (detail - 0.5f) * 6.0f * (1.0f - erosion * 0.4f);
    } else if (cont < 0.76f) {
        // 4. Highlands & Elevated Plateaus (Y=37-51)
        const float t = (cont - 0.62f) / 0.14f;
        rawH = 37.0f + t * 14.0f + (detail - 0.5f) * 6.0f;
    } else {
        // 5. High Mountains & Alpine Peaks (Towering peaks up to Y=76)
        const float t = (cont - 0.76f) / 0.24f;
        float ridge = (fnlPeaks.GetNoise(wx, wz) + 1.0f) * 0.5f;
        ridge = ridge * ridge;
        rawH = 51.0f + t * 11.0f + ridge * 14.0f + (detail - 0.5f) * 4.0f;
    }

    // 6. Meandering Rivers (carved smoothly through landforms)
    if (cont >= 0.30f) {
        const float riverVal = sampleRiver(wx, wz, seed);
        const float riverWidth = 0.024f;
        if (riverVal < riverWidth) {
            float riverFactor = 1.0f - (riverVal / riverWidth);
            riverFactor = riverFactor * riverFactor * (3.0f - 2.0f * riverFactor);
            const float riverBedH = 21.0f + (detail - 0.5f) * 3.0f;
            rawH = glm::mix(rawH, riverBedH, riverFactor * 0.90f);
        }
    }

    return std::clamp(rawH, 1.0f, static_cast<float>(Chunk::H - 4));
}

void TerrainGenerator::plantTreeInChunk(Chunk& chunk, int originX, int originZ, int x, int groundY, int z, float cont) const {
    const float styleRoll = hash2(x, z, m_seed + 1337);
    const float varRoll = hash2(x, z, m_seed + 9821);

    auto putLeaf = [&](int bx, int by, int bz) {
        if (by >= 0 && by < Chunk::H) {
            const int lx = bx - originX;
            const int lz = bz - originZ;
            if (lx >= 0 && lx < Chunk::W && lz >= 0 && lz < Chunk::D) {
                if (chunk.get(lx, by, lz) == BlockId::Air) {
                    chunk.set(lx, by, lz, BlockId::Leaves);
                }
            }
        }
    };

    auto putLog = [&](int bx, int by, int bz) {
        if (by >= 0 && by < Chunk::H) {
            const int lx = bx - originX;
            const int lz = bz - originZ;
            if (lx >= 0 && lx < Chunk::W && lz >= 0 && lz < Chunk::D) {
                chunk.set(lx, by, lz, BlockId::Wood);
            }
        }
    };

    // Highlands and Mountain zones favor tall conical spruce / pine trees
    const bool isHighAltitude = (cont >= 0.65f);
    const bool usePine = isHighAltitude ? (styleRoll < 0.75f) : (styleRoll < 0.20f);

    if (usePine) {
        // --- 1. Conical Pine / Spruce Tree (Tiered silhouette) ---
        const int trunk = 6 + static_cast<int>(varRoll * 3.0f); // Height 6-8
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        // Tier 1 (bottom wide ring)
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dz = -2; dz <= 2; ++dz) {
                if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;
                putLeaf(x + dx, topY - 4, z + dz);
            }
        }
        // Tier 2 (small ring)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                putLeaf(x + dx, topY - 3, z + dz);
            }
        }
        // Tier 3 (mid wide ring)
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dz = -2; dz <= 2; ++dz) {
                if (std::abs(dx) + std::abs(dz) > 2) continue;
                putLeaf(x + dx, topY - 2, z + dz);
            }
        }
        // Tier 4 (small ring)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                putLeaf(x + dx, topY - 1, z + dz);
            }
        }
        // Top cross around trunk top
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dz == 0) continue;
                putLeaf(x + dx, topY, z + dz);
            }
        }
        // Peak cross over trunk
        putLeaf(x, topY + 1, z);
        putLeaf(x + 1, topY + 1, z);
        putLeaf(x - 1, topY + 1, z);
        putLeaf(x, topY + 1, z + 1);
        putLeaf(x, topY + 1, z - 1);
        putLeaf(x, topY + 2, z);

    } else if (styleRoll < 0.48f) {
        // --- 2. Tall Forest Oak (Tall trunk, large round canopy) ---
        const int trunk = 6 + static_cast<int>(varRoll * 3.0f);
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        for (int dy = -3; dy <= 2; ++dy) {
            int radius = 2;
            if (dy == -3 || dy == 1) radius = 2;
            if (dy == 2) radius = 1;

            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dz = -radius; dz <= radius; ++dz) {
                    if (dy <= 0 && dx == 0 && dz == 0) continue;
                    if (radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2) {
                        if (hash2(x + dx, z + dz, m_seed + static_cast<uint32_t>(dy) * 31) > 0.4f) continue;
                    }
                    if (dy == 2 && std::abs(dx) == 1 && std::abs(dz) == 1) continue;
                    putLeaf(x + dx, topY + dy, z + dz);
                }
            }
        }
        putLeaf(x, topY + 1, z);
        putLeaf(x, topY + 2, z);

    } else if (styleRoll < 0.70f) {
        // --- 3. Short Bushy / Apple Tree (Dense low canopy) ---
        const int trunk = 3 + static_cast<int>(varRoll * 2.0f);
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        for (int dy = -1; dy <= 2; ++dy) {
            const int radius = (dy == 2) ? 1 : 2;
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dz = -radius; dz <= radius; ++dz) {
                    if (dy <= 0 && dx == 0 && dz == 0) continue;
                    if (radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2 && dy >= 1) continue;
                    putLeaf(x + dx, topY + dy, z + dz);
                }
            }
        }
        putLeaf(x, topY + 1, z);
        putLeaf(x, topY + 2, z);

    } else {
        // --- 4. Classic Oak Tree (Varied height 4-6, balanced rounded canopy) ---
        const int trunk = 4 + static_cast<int>(varRoll * 3.0f);
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        for (int dy = -2; dy <= -1; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                for (int dz = -2; dz <= 2; ++dz) {
                    if (dx == 0 && dz == 0) continue;
                    if (std::abs(dx) == 2 && std::abs(dz) == 2) {
                        if (hash2(x + dx, z + dz, m_seed + static_cast<uint32_t>(dy) * 17) > 0.5f) continue;
                    }
                    putLeaf(x + dx, topY + dy, z + dz);
                }
            }
        }

        for (int dx = -2; dx <= 2; ++dx) {
            for (int dz = -2; dz <= 2; ++dz) {
                if (dx == 0 && dz == 0) continue;
                if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;
                putLeaf(x + dx, topY, z + dz);
            }
        }

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (std::abs(dx) == 1 && std::abs(dz) == 1) {
                    if (varRoll > 0.6f) continue;
                }
                putLeaf(x + dx, topY + 1, z + dz);
            }
        }

        putLeaf(x, topY + 2, z);
        if (varRoll > 0.4f) {
            putLeaf(x + 1, topY + 2, z);
            putLeaf(x - 1, topY + 2, z);
            putLeaf(x, topY + 2, z + 1);
            putLeaf(x, topY + 2, z - 1);
        }
    }
}

void TerrainGenerator::generateChunk(World& world, Chunk& chunk, WorldType type) const {
    (void)world;
    const int ox = chunk.originX();
    const int oz = chunk.originZ();

    const int seaLevel = (type == WorldType::Flat) ? 0 : 26;
    const float caveThreshold = (type == WorldType::Cavernous) ? 0.025f : 0.012f;

    FastNoiseLite cave1 = makeCaveNoise1(m_seed);
    FastNoiseLite cave2 = makeCaveNoise2(m_seed);

    // 1. Carve terrain & voxel columns for this chunk
    for (int lz = 0; lz < Chunk::D; ++lz) {
        const int wz = oz + lz;
        for (int lx = 0; lx < Chunk::W; ++lx) {
            const int wx = ox + lx;

            const float fHeight = sampleTerrainHeight(static_cast<float>(wx), static_cast<float>(wz), m_seed, type);
            const int height = std::clamp(static_cast<int>(std::floor(fHeight)), 1, Chunk::H - 3);
            const float cont = sampleContinentalness(static_cast<float>(wx), static_cast<float>(wz), m_seed);

            const int maxY = std::max(height, seaLevel);

            for (int y = 0; y <= maxY; ++y) {
                if (y == 0) {
                    chunk.set(lx, y, lz, BlockId::Bedrock);
                    continue;
                }

                if (y > height) {
                    chunk.set(lx, y, lz, BlockId::Water);
                    continue;
                }

                BlockId block;
                const bool isBeach = (height <= seaLevel + 1 && type != WorldType::Flat);
                const bool isMountainPeak = (height >= 56 && cont >= 0.76f && type != WorldType::Flat);

                if (y == height) {
                    if (isBeach) {
                        block = BlockId::Sand;
                    } else if (isMountainPeak) {
                        block = BlockId::Stone; // Rocky mountain summits
                    } else {
                        block = BlockId::Grass;
                    }
                } else if (y >= height - 3) {
                    if (isBeach) {
                        block = BlockId::Sand;
                    } else if (isMountainPeak) {
                        block = BlockId::Cobblestone;
                    } else {
                        block = BlockId::Dirt;
                    }
                } else {
                    const float oreRoll = hash3(wx, y, wz, m_seed + 101);
                    if (y < 12 && oreRoll > 0.988f) {
                        block = BlockId::DiamondOre;
                    } else if (y < 22 && oreRoll > 0.978f) {
                        block = BlockId::GoldOre;
                    } else if (y < 38 && oreRoll > 0.940f) {
                        block = BlockId::IronOre;
                    } else if (oreRoll > 0.880f) {
                        block = BlockId::CoalOre;
                    } else {
                        block = BlockId::Stone;
                    }
                }

                // 3D Caves (Noodle caverns & cheese caves)
                if (type != WorldType::Flat && y >= 4 && y < height - 3) {
                    const float c1 = (cave1.GetNoise(static_cast<float>(wx), static_cast<float>(y) * 1.25f, static_cast<float>(wz)) + 1.0f) * 0.5f;
                    const float c2 = (cave2.GetNoise(static_cast<float>(wx), static_cast<float>(y) * 1.25f, static_cast<float>(wz)) + 1.0f) * 0.5f;
                    const float dist = (c1 - 0.5f) * (c1 - 0.5f) + (c2 - 0.5f) * (c2 - 0.5f);

                    if (dist < caveThreshold || (c1 > 0.74f && c2 > 0.70f)) {
                        if (y <= seaLevel) {
                            block = BlockId::Water;
                        } else {
                            block = BlockId::Air;
                        }
                    }
                }

                chunk.set(lx, y, lz, block);
            }
        }
    }

    // 2. Scatter trees deterministically (check tree roots in [ox - 2, ox + Chunk::W + 1] x [oz - 2, oz + Chunk::D + 1])
    if (type != WorldType::Flat) {
        for (int tz = oz - 2; tz <= oz + Chunk::D + 1; ++tz) {
            for (int tx = ox - 2; tx <= ox + Chunk::W + 1; ++tx) {
                const float fHeight = sampleTerrainHeight(static_cast<float>(tx), static_cast<float>(tz), m_seed, type);
                const int surfY = std::clamp(static_cast<int>(std::floor(fHeight)), 1, Chunk::H - 3);

                if (surfY >= seaLevel) {
                    const float cont = sampleContinentalness(static_cast<float>(tx), static_cast<float>(tz), m_seed);
                    const bool isBeach = (surfY <= seaLevel + 1);
                    const bool isMountainPeak = (surfY >= 56 && cont >= 0.76f);

                    if (!isBeach && !isMountainPeak) {
                        const float treeNoise = fbm(static_cast<float>(tx) * 0.08f, static_cast<float>(tz) * 0.08f, m_seed + 333);
                        const float treeChance = (cont >= 0.45f && cont < 0.65f && treeNoise > 0.55f) ? 0.965f : 0.991f;

                        if (hash2(tx, tz, m_seed + 555) > treeChance) {
                            plantTreeInChunk(chunk, ox, oz, tx, surfY, tz, cont);
                        }
                    }
                }
            }
        }
    }

    // 3. Scatter wild tall grass
    if (type != WorldType::Flat) {
        for (int lz = 0; lz < Chunk::D; ++lz) {
            const int wz = oz + lz;
            for (int lx = 0; lx < Chunk::W; ++lx) {
                const int wx = ox + lx;
                int surf = 0;
                for (int y = Chunk::H - 1; y >= 0; --y) {
                    if (chunk.get(lx, y, lz) != BlockId::Air) {
                        surf = y;
                        break;
                    }
                }

                if (surf >= seaLevel && chunk.get(lx, surf, lz) == BlockId::Grass) {
                    if (surf + 1 < Chunk::H && chunk.get(lx, surf + 1, lz) == BlockId::Air) {
                        const float grassNoise = fbm(static_cast<float>(wx) * 0.10f, static_cast<float>(wz) * 0.10f, m_seed + 777);
                        const float scatterRoll = hash2(wx, wz, m_seed + 888);
                        if (grassNoise > 0.40f && scatterRoll < 0.45f) {
                            chunk.set(lx, surf + 1, lz, BlockId::TallGrass);
                        }
                    }
                }
            }
        }
    }
}

void TerrainGenerator::generateInitialSpawn(World& world, int radiusInChunks, const ProgressCallback& onProgress) const {
    if (onProgress) onProgress(0.10f, "Generating spawn area chunks...");

    const int minC = -radiusInChunks;
    const int maxC = radiusInChunks;
    const int total = (maxC - minC + 1) * (maxC - minC + 1);
    int count = 0;

    for (int cz = minC; cz <= maxC; ++cz) {
        for (int cx = minC; cx <= maxC; ++cx) {
            world.generateSingleChunk(cx, cz, m_type);
            count++;
            if (onProgress && (count % 4 == 0 || count == total)) {
                float p = 0.10f + 0.60f * (static_cast<float>(count) / static_cast<float>(total));
                onProgress(p, "Generating terrain chunk (" + std::to_string(count) + "/" + std::to_string(total) + ")...");
            }
        }
    }

    world.rebuildLoadedList();

    if (onProgress) onProgress(0.75f, "Founding Pigman Villages & Settlements...");
    if (m_type != WorldType::Cavernous) {
        VillageGenerator::generateVillages(world, m_seed, world.villages());
    }

    world.rebuildLoadedList();

    if (onProgress) onProgress(0.85f, "Computing world lighting...");
    world.computeWorldLighting(onProgress);
}

void TerrainGenerator::generate(World& world, const ProgressCallback& onProgress) const {
    generateInitialSpawn(world, 2, onProgress);
}

} // namespace vox
