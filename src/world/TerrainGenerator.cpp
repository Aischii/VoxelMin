#include "world/TerrainGenerator.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/VillageGenerator.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vox {

float TerrainGenerator::hash2(int x, int z, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393U +
                 static_cast<uint32_t>(z) * 668265263U +
                 seed * 362437U;
    h = (h ^ (h >> 13)) * 1274126177U;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

float TerrainGenerator::valueNoise(float x, float z, uint32_t seed) {
    const int xi = static_cast<int>(std::floor(x));
    const int zi = static_cast<int>(std::floor(z));
    const float xf = x - static_cast<float>(xi);
    const float zf = z - static_cast<float>(zi);

    const float u = xf * xf * (3.0f - 2.0f * xf);
    const float v = zf * zf * (3.0f - 2.0f * zf);

    const float a = hash2(xi, zi, seed);
    const float b = hash2(xi + 1, zi, seed);
    const float c = hash2(xi, zi + 1, seed);
    const float d = hash2(xi + 1, zi + 1, seed);

    const float top = a + (b - a) * u;
    const float bottom = c + (d - c) * u;
    return top + (bottom - top) * v;
}

float TerrainGenerator::fbm(float x, float z, uint32_t seed) {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int octave = 0; octave < 4; ++octave) {
        total += valueNoise(x * frequency, z * frequency, seed + static_cast<uint32_t>(octave) * 1013U) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return total / maxValue;
}

void TerrainGenerator::plantTree(World& world, int x, int groundY, int z) const {
    const float styleRoll = hash2(x, z, m_seed + 1337);
    const float varRoll = hash2(x, z, m_seed + 9821);

    auto putLeaf = [&](int lx, int ly, int lz) {
        if (ly >= 0 && ly < Chunk::H && lx >= 0 && lx < world.widthBlocks() && lz >= 0 && lz < world.depthBlocks()) {
            if (world.getBlock(lx, ly, lz) == BlockId::Air) {
                world.setBlock(lx, ly, lz, BlockId::Leaves);
            }
        }
    };

    auto putLog = [&](int lx, int ly, int lz) {
        if (ly >= 0 && ly < Chunk::H && lx >= 0 && lx < world.widthBlocks() && lz >= 0 && lz < world.depthBlocks()) {
            world.setBlock(lx, ly, lz, BlockId::Wood);
        }
    };

    if (styleRoll < 0.20f) {
        // --- 1. Conical Pine / Spruce Tree (Tiered silhouette) ---
        const int trunk = 6 + static_cast<int>(varRoll * 3.0f); // Height 6-8
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        // Tiered pine foliage
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
                if (std::abs(dx) + std::abs(dz) > 2) continue; // Diamond/cross
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
                if (dx == 0 && dz == 0) continue; // Trunk is here
                putLeaf(x + dx, topY, z + dz);
            }
        }
        // Peak cross over trunk
        putLeaf(x, topY + 1, z);
        putLeaf(x + 1, topY + 1, z);
        putLeaf(x - 1, topY + 1, z);
        putLeaf(x, topY + 1, z + 1);
        putLeaf(x, topY + 1, z - 1);
        // Spire tip
        putLeaf(x, topY + 2, z);

    } else if (styleRoll < 0.45f) {
        // --- 2. Tall Forest Oak (Tall trunk, large round canopy) ---
        const int trunk = 6 + static_cast<int>(varRoll * 3.0f); // Height 6-8
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        // Big spherical canopy
        for (int dy = -3; dy <= 2; ++dy) {
            int radius = 2;
            if (dy == -3 || dy == 1) radius = 2;
            if (dy == 2) radius = 1;

            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dz = -radius; dz <= radius; ++dz) {
                    if (dy <= 0 && dx == 0 && dz == 0) continue; // Keep trunk inside
                    if (radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2) {
                        // Soften corners
                        if (hash2(x + dx, z + dz, m_seed + static_cast<uint32_t>(dy) * 31) > 0.4f) continue;
                    }
                    if (dy == 2 && std::abs(dx) == 1 && std::abs(dz) == 1) continue;
                    putLeaf(x + dx, topY + dy, z + dz);
                }
            }
        }
        // Ensure direct top cap
        putLeaf(x, topY + 1, z);
        putLeaf(x, topY + 2, z);

    } else if (styleRoll < 0.65f) {
        // --- 3. Short Bushy / Apple Tree (Dense low canopy) ---
        const int trunk = 3 + static_cast<int>(varRoll * 2.0f); // Height 3-4
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
        const int trunk = 4 + static_cast<int>(varRoll * 3.0f); // Height 4-6
        for (int i = 1; i <= trunk; ++i) {
            putLog(x, groundY + i, z);
        }
        const int topY = groundY + trunk;

        // Base layers (dy = -2 and -1): 5x5 with cut corners
        for (int dy = -2; dy <= -1; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                for (int dz = -2; dz <= 2; ++dz) {
                    if (dx == 0 && dz == 0) continue;
                    if (std::abs(dx) == 2 && std::abs(dz) == 2) {
                        // Natural irregular corner trimming
                        if (hash2(x + dx, z + dz, m_seed + static_cast<uint32_t>(dy) * 17) > 0.5f) continue;
                    }
                    putLeaf(x + dx, topY + dy, z + dz);
                }
            }
        }

        // Mid layer (dy = 0, level with top of trunk): 3x3 + side puffs
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dz = -2; dz <= 2; ++dz) {
                if (dx == 0 && dz == 0) continue;
                if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;
                putLeaf(x + dx, topY, z + dz);
            }
        }

        // Upper layer (dy = 1, directly over trunk): 3x3 with trimmed corners, center leaf present!
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (std::abs(dx) == 1 && std::abs(dz) == 1) {
                    if (varRoll > 0.6f) continue;
                }
                putLeaf(x + dx, topY + 1, z + dz);
            }
        }

        // Top crown (dy = 2): cross shape
        putLeaf(x, topY + 2, z);
        if (varRoll > 0.4f) {
            putLeaf(x + 1, topY + 2, z);
            putLeaf(x - 1, topY + 2, z);
            putLeaf(x, topY + 2, z + 1);
            putLeaf(x, topY + 2, z - 1);
        }
    }
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
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));
    const int zi = static_cast<int>(std::floor(z));
    const float xf = x - static_cast<float>(xi);
    const float yf = y - static_cast<float>(yi);
    const float zf = z - static_cast<float>(zi);

    const float u = xf * xf * (3.0f - 2.0f * xf);
    const float v = yf * yf * (3.0f - 2.0f * yf);
    const float w = zf * zf * (3.0f - 2.0f * zf);

    const float c000 = hash3(xi, yi, zi, seed);
    const float c100 = hash3(xi + 1, yi, zi, seed);
    const float c010 = hash3(xi, yi + 1, zi, seed);
    const float c110 = hash3(xi + 1, yi + 1, zi, seed);
    const float c001 = hash3(xi, yi, zi + 1, seed);
    const float c101 = hash3(xi + 1, yi, zi + 1, seed);
    const float c011 = hash3(xi, yi + 1, zi + 1, seed);
    const float c111 = hash3(xi + 1, yi + 1, zi + 1, seed);

    const float c00 = c000 + (c100 - c000) * u;
    const float c10 = c010 + (c110 - c010) * u;
    const float c01 = c001 + (c101 - c001) * u;
    const float c11 = c011 + (c111 - c011) * u;

    const float c0 = c00 + (c10 - c00) * v;
    const float c1 = c01 + (c11 - c01) * v;

    return c0 + (c1 - c0) * w;
}

void TerrainGenerator::generate(World& world, const ProgressCallback& onProgress) const {
    const int width = world.widthBlocks();
    const int depth = world.depthBlocks();

    if (onProgress) onProgress(0.05f, "Carving terrain & surface topography...");

    // World type characteristics
    int seaLevel = 26;
    float heightScale = 18.0f;
    float baseHeight = 24.0f;
    float caveThreshold = 0.012f;

    if (m_type == WorldType::Flat) {
        seaLevel = 0;
        baseHeight = 4.0f;
        heightScale = 0.0f;
    } else if (m_type == WorldType::Mountainous) {
        seaLevel = 22;
        baseHeight = 28.0f;
        heightScale = 36.0f;
    } else if (m_type == WorldType::Cavernous) {
        seaLevel = 24;
        baseHeight = 26.0f;
        heightScale = 16.0f;
        caveThreshold = 0.025f;
    } else if (m_type == WorldType::Island) {
        seaLevel = 28;
        baseHeight = 32.0f;
        heightScale = 14.0f;
    } else {
        // Default seeded characteristics
        const float typeRoll = hash2(0, 0, m_seed + 7777);
        seaLevel = (typeRoll < 0.25f) ? 24 : ((typeRoll < 0.50f) ? 27 : 26);
        heightScale = (typeRoll < 0.25f) ? 14.0f : ((typeRoll < 0.50f) ? 22.0f : 18.0f);
        baseHeight = (typeRoll < 0.25f) ? 26.0f : ((typeRoll < 0.50f) ? 22.0f : 24.0f);
    }

    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerZ = static_cast<float>(depth) * 0.5f;
    const float maxRadius = static_cast<float>(width) * 0.46f;

    for (int z = 0; z < depth; ++z) {
        if (onProgress && (z % 64 == 0)) {
            float p = 0.05f + 0.45f * (static_cast<float>(z) / static_cast<float>(depth));
            onProgress(p, "Carving terrain & subterranean layers...");
        }

        for (int x = 0; x < width; ++x) {
            int height = 4;

            if (m_type == WorldType::Flat) {
                height = 4;
            } else {
                const float n = fbm(static_cast<float>(x) * 0.032f, static_cast<float>(z) * 0.032f, m_seed);
                float rawH = baseHeight + n * heightScale;

                if (m_type == WorldType::Island) {
                    const float dx = static_cast<float>(x) - centerX;
                    const float dz = static_cast<float>(z) - centerZ;
                    const float dist = std::sqrt(dx * dx + dz * dz) / maxRadius;
                    const float mask = std::clamp((1.0f - dist) * 1.5f, 0.0f, 1.0f);
                    rawH = 16.0f + (rawH - 16.0f) * mask;
                }

                height = static_cast<int>(rawH);
                height = std::clamp(height, 1, Chunk::H - 3);
            }

            const int maxY = std::max(height, seaLevel);

            for (int y = 0; y <= maxY; ++y) {
                if (y == 0) {
                    world.setBlock(x, y, z, BlockId::Bedrock);
                    continue;
                }

                if (y > height) {
                    // Water layer filling up to sea level
                    world.setBlock(x, y, z, BlockId::Water);
                    continue;
                }

                // Sub-surface and surface block determination
                BlockId block;
                const bool isBeach = (height <= seaLevel + 1 && m_type != WorldType::Flat);

                if (y == height) {
                    block = isBeach ? BlockId::Sand : BlockId::Grass;
                } else if (y >= height - 3) {
                    block = isBeach ? BlockId::Sand : BlockId::Dirt;
                } else {
                    // Underground stone & mineral ores
                    const float coalRoll = hash3(x, y, z, m_seed + 101);
                    const float ironRoll = hash3(x, y, z, m_seed + 102);
                    const float goldRoll = hash3(x, y, z, m_seed + 103);
                    const float diaRoll = hash3(x, y, z, m_seed + 104);

                    if (y < 12 && diaRoll > 0.988f) {
                        block = BlockId::DiamondOre;
                    } else if (y < 22 && goldRoll > 0.980f) {
                        block = BlockId::GoldOre;
                    } else if (y < 38 && ironRoll > 0.965f) {
                        block = BlockId::IronOre;
                    } else if (coalRoll > 0.950f) {
                        block = BlockId::CoalOre;
                    } else {
                        block = BlockId::Stone;
                    }
                }

                // 3D Cave generation (worm tunnels & caverns)
                if (m_type != WorldType::Flat && y >= 4 && y < height - 3) {
                    const float nx = static_cast<float>(x) * 0.065f;
                    const float ny = static_cast<float>(y) * 0.085f;
                    const float nz = static_cast<float>(z) * 0.065f;
                    const float c1 = noise3D(nx, ny, nz, m_seed + 501);
                    const float c2 = noise3D(nx, ny, nz, m_seed + 607);
                    const float dist = (c1 - 0.5f) * (c1 - 0.5f) + (c2 - 0.5f) * (c2 - 0.5f);

                    if (dist < caveThreshold || (c1 > 0.74f && c2 > 0.70f)) {
                        if (y <= seaLevel) {
                            block = BlockId::Water; // Flooded cave pocket
                        } else {
                            block = BlockId::Air;   // Open cave tunnel
                        }
                    }
                }

                world.setBlock(x, y, z, block);
            }
        }
    }

    if (onProgress) onProgress(0.55f, "Lighting subterranean cave alcoves...");

    // Place torches in dark cave alcoves
    for (int z = 4; z < depth - 4; ++z) {
        for (int x = 4; x < width - 4; ++x) {
            for (int y = 4; y < seaLevel + 15; ++y) {
                if (world.getBlock(x, y, z) == BlockId::Air) {
                    const BlockId below = world.getBlock(x, y - 1, z);
                    if (below == BlockId::Stone || below == BlockId::Cobblestone ||
                        below == BlockId::CoalOre || below == BlockId::IronOre) {
                        if (hash3(x, y, z, m_seed + 999) > 0.993f) {
                            world.setBlock(x, y, z, BlockId::Torch);
                        }
                    }
                }
            }
        }
    }

    if (onProgress) onProgress(0.68f, "Planting ancient forests & flora...");

    // Scatter trees on grass columns above water level
    for (int z = 3; z < depth - 3; ++z) {
        for (int x = 3; x < width - 3; ++x) {
            const int surface = world.surfaceHeight(x, z);
            if (surface >= seaLevel && world.getBlock(x, surface, z) == BlockId::Grass) {
                if (hash2(x, z, m_seed + 555) > 0.992f) {
                    plantTree(world, x, surface, z);
                }
            }
        }
    }

    if (onProgress) onProgress(0.80f, "Cultivating wild tall grass...");

    // Scatter wild tall grass across surface grass terrain in natural clusters
    if (m_type != WorldType::Flat) {
        for (int z = 2; z < depth - 2; ++z) {
            for (int x = 2; x < width - 2; ++x) {
                const int surface = world.surfaceHeight(x, z);
                if (surface >= seaLevel && world.getBlock(x, surface, z) == BlockId::Grass) {
                    if (surface + 1 < Chunk::H && world.getBlock(x, surface + 1, z) == BlockId::Air) {
                        const float grassNoise = fbm(static_cast<float>(x) * 0.10f, static_cast<float>(z) * 0.10f, m_seed + 777);
                        const float scatterRoll = hash2(x, z, m_seed + 888);
                        if (grassNoise > 0.42f && scatterRoll < 0.40f) {
                            world.setBlock(x, surface + 1, z, BlockId::TallGrass);
                        }
                    }
                }
            }
        }
    }

    if (onProgress) onProgress(0.88f, "Founding Pigman Villages & Settlements...");

    // Generate multi-template surface-conforming Pigman Villages
    if (m_type != WorldType::Cavernous) {
        VillageGenerator::generateVillages(world, m_seed, world.villages());
    }

    if (onProgress) onProgress(0.96f, "Finalizing world terrain...");
}

} // namespace vox
