#include "render/Texture.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "world/Block.hpp"

#include <GL/glew.h>

#if __has_include(<stb/stb_image.h>)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#elif __has_include(<stb_image.h>)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#if __has_include(<stb/stb_image_resize2.h>)
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include <stb/stb_image_resize2.h>
#define VOX_USE_STB_RESIZE2 1
#elif __has_include(<stb_image_resize2.h>)
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include <stb_image_resize2.h>
#define VOX_USE_STB_RESIZE2 1
#elif __has_include(<stb/stb_image_resize.h>)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
#define VOX_USE_STB_RESIZE1 1
#elif __has_include(<stb_image_resize.h>)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#define VOX_USE_STB_RESIZE1 1
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace vox {
namespace {

constexpr int TILE = config::TILE_PIXELS;
constexpr int TILES = config::ATLAS_TILES;
constexpr int SIZE = config::ATLAS_PIXELS;

struct Rgb {
    float r, g, b;
};

// Deterministic hash -> [0, 1). Used to add per-pixel noise without any RNG
// state, so textures are identical every run.
uint32_t hashU(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float rnd(int x, int y, uint32_t seed) {
    const uint32_t h = hashU(static_cast<uint32_t>(x) * 374761393U +
                             static_cast<uint32_t>(y) * 668265263U +
                             seed * 362437U);
    return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}

uint8_t toByte(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return static_cast<uint8_t>(v * 255.0f + 0.5f);
}

// Writes one pixel of a tile. Tile-local y = 0 is the *bottom* row, matching
// OpenGL's convention (buffer row 0 becomes v = 0).
void setPixel(std::vector<uint8_t>& pixels, int tile, int x, int y, Rgb c) {
    const int tx = (tile % TILES) * TILE + x;
    const int ty = (tile / TILES) * TILE + y;
    const size_t i = (static_cast<size_t>(ty) * SIZE + tx) * 4;
    pixels[i + 0] = toByte(c.r);
    pixels[i + 1] = toByte(c.g);
    pixels[i + 2] = toByte(c.b);
    pixels[i + 3] = 255;
}

void fillNoise(std::vector<uint8_t>& pixels, int tile, Rgb base, float amplitude, uint32_t seed) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, seed) - 0.5f) * 2.0f * amplitude;
            setPixel(pixels, tile, x, y, {base.r + n, base.g + n, base.b + n});
        }
    }
}

void makeGrassSide(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.52f, 0.37f, 0.26f}, 0.06f, 11);
    for (int x = 0; x < TILE; ++x) {
        const int depth = 3 + static_cast<int>(rnd(x, 7, 123) * 4.0f);
        for (int y = TILE - depth; y < TILE; ++y) {
            const float n = (rnd(x, y, 21) - 0.5f) * 0.12f;
            setPixel(pixels, tile, x, y, {0.37f + n, 0.62f + n, 0.21f + n});
        }
    }
}

void makeDirtPathTop(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.62f, 0.48f, 0.32f}, 0.05f, 41);
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float rVal = rnd(x, y, 79);
            if (rVal < 0.12f) {
                setPixel(pixels, tile, x, y, {0.52f, 0.39f, 0.25f});
            } else if (rVal > 0.88f) {
                setPixel(pixels, tile, x, y, {0.68f, 0.55f, 0.38f});
            }
        }
    }
}

void makeDirtPathSide(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.52f, 0.37f, 0.26f}, 0.06f, 42);
    for (int x = 0; x < TILE; ++x) {
        const int pathDepth = 2 + static_cast<int>(rnd(x, 3, 81) * 2.0f);
        for (int y = TILE - pathDepth; y < TILE; ++y) {
            const float n = (rnd(x, y, 99) - 0.5f) * 0.08f;
            setPixel(pixels, tile, x, y, {0.62f + n, 0.48f + n, 0.32f + n});
        }
    }
}

void makeWoodSide(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.40f, 0.30f, 0.18f}, 0.05f, 31);
    for (int x = 0; x < TILE; ++x) {
        const float stripe = (rnd(x, 0, 55) < 0.3f) ? 0.82f : 1.0f;
        for (int y = 0; y < TILE; ++y) {
            const float n = (rnd(x, y, 57) - 0.5f) * 0.06f;
            setPixel(pixels, tile, x, y, {0.40f * stripe + n, 0.30f * stripe + n, 0.18f * stripe + n});
        }
    }
}

void makeWoodTop(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            if (x < 2 || x >= TILE - 2 || y < 2 || y >= TILE - 2) {
                const float n = (rnd(x, y, 31) - 0.5f) * 0.06f;
                const float stripe = (rnd(x, y, 55) < 0.3f) ? 0.82f : 1.0f;
                setPixel(pixels, tile, x, y, {0.40f * stripe + n, 0.30f * stripe + n, 0.18f * stripe + n});
                continue;
            }
            const float dx = static_cast<float>(x) - 7.5f;
            const float dy = static_cast<float>(y) - 7.5f;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float ring = (std::fmod(d, 2.5f) < 1.25f) ? 0.85f : 1.05f;
            const float n = (rnd(x, y, 41) - 0.5f) * 0.05f;
            setPixel(pixels, tile, x, y, {0.61f * ring + n, 0.49f * ring + n, 0.31f * ring + n});
        }
    }
}

void makeLeaves(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.24f, 0.47f, 0.18f}, 0.10f, 77);
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            if (rnd(x, y, 79) < 0.12f) {
                setPixel(pixels, tile, x, y, {0.15f, 0.33f, 0.11f});
            }
        }
    }
}

void makePlanks(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        const float line = (y % 4 == 0) ? 0.78f : 1.0f;
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, 91) - 0.5f) * 0.05f;
            setPixel(pixels, tile, x, y, {0.63f * line + n, 0.47f * line + n, 0.27f * line + n});
        }
    }
}

void makeBedrock(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = rnd(x / 2, y / 2, 99);
            const float g = 0.22f + n * 0.24f;
            setPixel(pixels, tile, x, y, {g, g, g});
        }
    }
}

void makeStone(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.50f, 0.50f, 0.50f}, 0.05f, 63);
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            if (rnd(x, y, 65) < 0.08f) {
                const float g = 0.40f;
                setPixel(pixels, tile, x, y, {g, g, g});
            }
        }
    }
}

void setPixelRgba(std::vector<uint8_t>& pixels, int tile, int x, int y, Rgb c, float a) {
    const int tx = (tile % TILES) * TILE + x;
    const int ty = (tile / TILES) * TILE + y;
    const size_t i = (static_cast<size_t>(ty) * SIZE + tx) * 4;
    pixels[i + 0] = toByte(c.r);
    pixels[i + 1] = toByte(c.g);
    pixels[i + 2] = toByte(c.b);
    pixels[i + 3] = toByte(a);
}

void makeWater(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float wave = std::sin(static_cast<float>(x + y) * 0.8f) * 0.08f;
            const float n = (rnd(x, y, 101) - 0.5f) * 0.08f;
            const float r = 0.16f + wave + n;
            const float g = 0.38f + wave * 1.2f + n;
            const float b = 0.82f + wave * 0.5f + n;
            setPixelRgba(pixels, tile, x, y, {r, g, b}, 0.75f);
        }
    }
}

void makeTorch(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            setPixelRgba(pixels, tile, x, y, {0.0f, 0.0f, 0.0f}, 0.0f);
        }
    }
    // Wooden stick (x: 7..8, y: 1..9)
    for (int y = 1; y <= 9; ++y) {
        setPixelRgba(pixels, tile, 7, y, {0.42f, 0.30f, 0.16f}, 1.0f);
        setPixelRgba(pixels, tile, 8, y, {0.52f, 0.38f, 0.22f}, 1.0f);
    }
    // Charcoal head (x: 7..8, y: 10..11)
    for (int y = 10; y <= 11; ++y) {
        setPixelRgba(pixels, tile, 7, y, {0.18f, 0.15f, 0.12f}, 1.0f);
        setPixelRgba(pixels, tile, 8, y, {0.24f, 0.20f, 0.16f}, 1.0f);
    }
    // Flame (x: 6..9, y: 11..15)
    setPixelRgba(pixels, tile, 6, 12, {0.95f, 0.45f, 0.10f}, 1.0f);
    setPixelRgba(pixels, tile, 9, 12, {0.95f, 0.45f, 0.10f}, 1.0f);
    setPixelRgba(pixels, tile, 7, 12, {1.00f, 0.90f, 0.25f}, 1.0f);
    setPixelRgba(pixels, tile, 8, 12, {1.00f, 0.95f, 0.40f}, 1.0f);
    setPixelRgba(pixels, tile, 7, 13, {1.00f, 0.95f, 0.50f}, 1.0f);
    setPixelRgba(pixels, tile, 8, 13, {1.00f, 0.95f, 0.50f}, 1.0f);
    setPixelRgba(pixels, tile, 7, 14, {1.00f, 0.80f, 0.20f}, 1.0f);
    setPixelRgba(pixels, tile, 8, 14, {1.00f, 0.70f, 0.15f}, 1.0f);
    setPixelRgba(pixels, tile, 7, 15, {0.90f, 0.40f, 0.10f}, 1.0f);
}

void makeTallGrass(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            setPixelRgba(pixels, tile, x, y, {0.28f, 0.68f, 0.16f}, 0.0f);
        }
    }
    // Grass blades growing upwards with varied blade heights and natural green tints
    for (int x = 2; x <= 13; ++x) {
        const int h = 6 + static_cast<int>(rnd(x, 3, 401) * 8.0f); // 6 to 13 pixels tall
        for (int y = 0; y <= h && y < TILE; ++y) {
            const float n = (rnd(x, y, 402) - 0.5f) * 0.10f;
            const float tipShade = 1.0f - (static_cast<float>(y) / static_cast<float>(TILE)) * 0.22f;
            const float r = (0.28f + n) * tipShade;
            const float g = (0.68f + n) * tipShade;
            const float b = (0.16f + n) * tipShade;
            setPixelRgba(pixels, tile, x, y, {r, g, b}, 1.0f);
        }
    }
    // Curved side blade tips
    setPixelRgba(pixels, tile, 1, 4, {0.26f, 0.62f, 0.14f}, 1.0f);
    setPixelRgba(pixels, tile, 1, 5, {0.30f, 0.70f, 0.18f}, 1.0f);
    setPixelRgba(pixels, tile, 14, 5, {0.28f, 0.65f, 0.16f}, 1.0f);
    setPixelRgba(pixels, tile, 14, 6, {0.32f, 0.72f, 0.20f}, 1.0f);
}

void makeCobblestone(std::vector<uint8_t>& pixels, int tile) {
    makeStone(pixels, tile);
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            if ((x % 4 == 0 && (y % 8 < 4)) || (x % 4 == 2 && (y % 8 >= 4)) || y % 4 == 0) {
                const float n = rnd(x, y, 88);
                const float g = 0.28f + n * 0.10f;
                setPixel(pixels, tile, x, y, {g, g, g});
            }
        }
    }
}

void makeOre(std::vector<uint8_t>& pixels, int tile, Rgb oreColor, uint32_t oreSeed) {
    makeStone(pixels, tile);
    for (int y = 1; y < TILE - 1; ++y) {
        for (int x = 1; x < TILE - 1; ++x) {
            if (rnd(x, y, oreSeed) < 0.16f) {
                const float n = (rnd(x, y, oreSeed + 7) - 0.5f) * 0.10f;
                setPixel(pixels, tile, x, y, {
                    std::clamp(oreColor.r + n, 0.0f, 1.0f),
                    std::clamp(oreColor.g + n, 0.0f, 1.0f),
                    std::clamp(oreColor.b + n, 0.0f, 1.0f)
                });
            }
        }
    }
}

bool loadTilePng(std::vector<uint8_t>& pixels, int tile, const std::string& filename, Rgb tint = {1.0f, 1.0f, 1.0f}) {
    const std::string paths[] = {
        "assets/textures/" + filename,
        "../assets/textures/" + filename,
        "../../assets/textures/" + filename,
        "bin/assets/textures/" + filename
    };
    int w = 0, h = 0, channels = 0;
    stbi_uc* data = nullptr;
    for (const auto& p : paths) {
        data = stbi_load(p.c_str(), &w, &h, &channels, 4);
        if (data) break;
    }
    if (!data) return false;

    std::vector<uint8_t> resizedData;
    const uint8_t* srcPixels = data;
    if (w != TILE || h != TILE) {
        resizedData.resize(static_cast<size_t>(TILE) * TILE * 4);
#if defined(VOX_USE_STB_RESIZE2)
        stbir_resize_uint8_linear(data, w, h, 0, resizedData.data(), TILE, TILE, 0, STBIR_RGBA);
#elif defined(VOX_USE_STB_RESIZE1)
        stbir_resize_uint8(data, w, h, 0, resizedData.data(), TILE, TILE, 0, 4);
#else
        // Simple nearest-neighbor fallback if stb resize is not available
        for (int ry = 0; ry < TILE; ++ry) {
            const int sy = (ry * h) / TILE;
            for (int rx = 0; rx < TILE; ++rx) {
                const int sx = (rx * w) / TILE;
                const size_t sIdx = (static_cast<size_t>(sy) * w + sx) * 4;
                const size_t dIdx = (static_cast<size_t>(ry) * TILE + rx) * 4;
                resizedData[dIdx + 0] = data[sIdx + 0];
                resizedData[dIdx + 1] = data[sIdx + 1];
                resizedData[dIdx + 2] = data[sIdx + 2];
                resizedData[dIdx + 3] = data[sIdx + 3];
            }
        }
#endif
        srcPixels = resizedData.data();
    }

    const bool hasTint = (tint.r != 1.0f || tint.g != 1.0f || tint.b != 1.0f);

    for (int y = 0; y < TILE; ++y) {
        const int srcY = (TILE - 1 - y); // Flip vertically for OpenGL UV (bottom-up)
        for (int x = 0; x < TILE; ++x) {
            const size_t srcIdx = (static_cast<size_t>(srcY) * TILE + x) * 4;
            const int tx = (tile % TILES) * TILE + x;
            const int ty = (tile / TILES) * TILE + y;
            const size_t dstIdx = (static_cast<size_t>(ty) * SIZE + tx) * 4;
            if (hasTint) {
                pixels[dstIdx + 0] = toByte((srcPixels[srcIdx + 0] / 255.0f) * tint.r);
                pixels[dstIdx + 1] = toByte((srcPixels[srcIdx + 1] / 255.0f) * tint.g);
                pixels[dstIdx + 2] = toByte((srcPixels[srcIdx + 2] / 255.0f) * tint.b);
            } else {
                pixels[dstIdx + 0] = srcPixels[srcIdx + 0];
                pixels[dstIdx + 1] = srcPixels[srcIdx + 1];
                pixels[dstIdx + 2] = srcPixels[srcIdx + 2];
            }
            pixels[dstIdx + 3] = srcPixels[srcIdx + 3];
        }
    }
    stbi_image_free(data);
    return true;
}

void makePigSkin(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.94f, 0.65f, 0.65f}, 0.04f, 701);
}

void makePigFace(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.94f, 0.65f, 0.65f}, 0.04f, 702);
    // Eyes (white with black pupil)
    setPixel(pixels, tile, 2, 8, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 3, 8, {0.1f, 0.1f, 0.1f});
    setPixel(pixels, tile, 12, 8, {0.1f, 0.1f, 0.1f});
    setPixel(pixels, tile, 13, 8, {1.0f, 1.0f, 1.0f});

    // Snout
    for (int y = 3; y <= 6; ++y) {
        for (int x = 5; x <= 10; ++x) {
            setPixel(pixels, tile, x, y, {0.86f, 0.48f, 0.52f});
        }
    }
    // Nostrils
    setPixel(pixels, tile, 6, 4, {0.45f, 0.20f, 0.22f});
    setPixel(pixels, tile, 9, 4, {0.45f, 0.20f, 0.22f});
}

void makePigSnout(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.86f, 0.48f, 0.52f}, 0.03f, 703);
    setPixel(pixels, tile, 4, 7, {0.45f, 0.20f, 0.22f});
    setPixel(pixels, tile, 11, 7, {0.45f, 0.20f, 0.22f});
}

void makeCowSkin(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float r = rnd(x, y, 801);
            const bool isSpot = (x > 3 && x < 12 && y > 2 && y < 10) || (x < 4 && y > 11) || (x > 10 && y < 4);
            if (isSpot) {
                const float n = (r - 0.5f) * 0.05f;
                setPixel(pixels, tile, x, y, {0.20f + n, 0.16f + n, 0.14f + n});
            } else {
                const float n = (r - 0.5f) * 0.04f;
                setPixel(pixels, tile, x, y, {0.88f + n, 0.86f + n, 0.84f + n});
            }
        }
    }
}

void makeCowFace(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.24f, 0.18f, 0.16f}, 0.04f, 802);
    // White stripe on forehead
    for (int y = 7; y < TILE; ++y) {
        for (int x = 6; x <= 9; ++x) {
            setPixel(pixels, tile, x, y, {0.88f, 0.86f, 0.84f});
        }
    }
    // Eyes
    setPixel(pixels, tile, 2, 8, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 3, 8, {0.1f, 0.1f, 0.1f});
    setPixel(pixels, tile, 12, 8, {0.1f, 0.1f, 0.1f});
    setPixel(pixels, tile, 13, 8, {1.0f, 1.0f, 1.0f});

    // Muzzle / Snout
    for (int y = 1; y <= 5; ++y) {
        for (int x = 3; x <= 12; ++x) {
            setPixel(pixels, tile, x, y, {0.55f, 0.48f, 0.48f});
        }
    }
    // Nostrils
    setPixel(pixels, tile, 5, 3, {0.20f, 0.15f, 0.15f});
    setPixel(pixels, tile, 10, 3, {0.20f, 0.15f, 0.15f});
}

void makeCowHorns(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.78f, 0.74f, 0.65f}, 0.05f, 803);
}

void makePigmanSkin(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.92f, 0.62f, 0.62f}, 0.04f, 901);
}

void makePigmanFace(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.92f, 0.62f, 0.62f}, 0.04f, 902);

    // Pigman Eyes (white sclera with sharp pupil)
    setPixel(pixels, tile, 2, 9, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 3, 9, {0.05f, 0.05f, 0.05f});
    setPixel(pixels, tile, 12, 9, {0.05f, 0.05f, 0.05f});
    setPixel(pixels, tile, 13, 9, {1.0f, 1.0f, 1.0f});

    // Brow ridges
    setPixel(pixels, tile, 2, 10, {0.72f, 0.45f, 0.45f});
    setPixel(pixels, tile, 3, 10, {0.72f, 0.45f, 0.45f});
    setPixel(pixels, tile, 12, 10, {0.72f, 0.45f, 0.45f});
    setPixel(pixels, tile, 13, 10, {0.72f, 0.45f, 0.45f});

    // Pig snout protruding in center
    for (int y = 4; y <= 7; ++y) {
        for (int x = 5; x <= 10; ++x) {
            setPixel(pixels, tile, x, y, {0.84f, 0.46f, 0.50f});
        }
    }
    // Dark nostrils
    setPixel(pixels, tile, 6, 5, {0.40f, 0.18f, 0.20f});
    setPixel(pixels, tile, 9, 5, {0.40f, 0.18f, 0.20f});

    // Pig ears
    setPixel(pixels, tile, 1, 13, {0.80f, 0.45f, 0.48f});
    setPixel(pixels, tile, 1, 14, {0.75f, 0.40f, 0.42f});
    setPixel(pixels, tile, 14, 13, {0.80f, 0.45f, 0.48f});
    setPixel(pixels, tile, 14, 14, {0.75f, 0.40f, 0.42f});
}

void makePigmanTorso(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.92f, 0.62f, 0.62f}, 0.04f, 903);
    // Villager tunic / leather vest markings
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < TILE; ++x) {
            if (x < 3 || x >= 13 || y < 4) {
                const float n = (rnd(x, y, 905) - 0.5f) * 0.05f;
                setPixel(pixels, tile, x, y, {0.46f + n, 0.32f + n, 0.20f + n});
            }
        }
    }
    // Buckle / amulet
    setPixel(pixels, tile, 7, 7, {0.95f, 0.82f, 0.25f});
    setPixel(pixels, tile, 8, 7, {0.95f, 0.82f, 0.25f});
}

void makePigmanHoof(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.92f, 0.62f, 0.62f}, 0.04f, 904);
    // Dark brown/black cloven hooves on lower portion
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, 906) - 0.5f) * 0.04f;
            const float cleft = (x == 7 || x == 8) ? 0.70f : 1.0f;
            setPixel(pixels, tile, x, y, {(0.22f + n) * cleft, (0.16f + n) * cleft, (0.14f + n) * cleft});
        }
    }
}

void makePlayerHead(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.29f, 0.19f, 0.12f}, 0.04f, 950);
}

void makePlayerFace(std::vector<uint8_t>& pixels, int tile) {
    // 1. Base skin tone
    fillNoise(pixels, tile, {0.78f, 0.56f, 0.41f}, 0.035f, 951);

    // 2. Brown hair on upper head with fringe
    for (int y = 12; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, 952) - 0.5f) * 0.04f;
            setPixel(pixels, tile, x, y, {0.29f + n, 0.19f + n, 0.12f + n});
        }
    }
    // Hair bangs / sideburns
    for (int y = 9; y < 12; ++y) {
        for (int x = 0; x < 3; ++x) {
            const float n = (rnd(x, y, 952) - 0.5f) * 0.04f;
            setPixel(pixels, tile, x, y, {0.29f + n, 0.19f + n, 0.12f + n});
        }
        for (int x = 13; x < TILE; ++x) {
            const float n = (rnd(x, y, 952) - 0.5f) * 0.04f;
            setPixel(pixels, tile, x, y, {0.29f + n, 0.19f + n, 0.12f + n});
        }
    }
    setPixel(pixels, tile, 3, 11, {0.29f, 0.19f, 0.12f});
    setPixel(pixels, tile, 12, 11, {0.29f, 0.19f, 0.12f});

    // 3. Eyes (white sclera + blue/violet pupil)
    // Left eye: x = 3..5, y = 6..7
    setPixel(pixels, tile, 3, 7, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 4, 7, {0.20f, 0.22f, 0.55f});
    setPixel(pixels, tile, 3, 6, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 4, 6, {0.20f, 0.22f, 0.55f});

    // Right eye: x = 11..13, y = 6..7
    setPixel(pixels, tile, 11, 7, {0.20f, 0.22f, 0.55f});
    setPixel(pixels, tile, 12, 7, {1.0f, 1.0f, 1.0f});
    setPixel(pixels, tile, 11, 6, {0.20f, 0.22f, 0.55f});
    setPixel(pixels, tile, 12, 6, {1.0f, 1.0f, 1.0f});

    // 4. Nose
    setPixel(pixels, tile, 7, 5, {0.68f, 0.46f, 0.33f});
    setPixel(pixels, tile, 8, 5, {0.68f, 0.46f, 0.33f});

    // 5. Classic Steve beard / smile
    for (int x = 5; x <= 10; ++x) {
        setPixel(pixels, tile, x, 3, {0.29f, 0.19f, 0.12f});
    }
    setPixel(pixels, tile, 5, 4, {0.29f, 0.19f, 0.12f});
    setPixel(pixels, tile, 10, 4, {0.29f, 0.19f, 0.12f});
    setPixel(pixels, tile, 7, 4, {0.52f, 0.28f, 0.24f});
    setPixel(pixels, tile, 8, 4, {0.52f, 0.28f, 0.24f});
}

void makePlayerTorso(std::vector<uint8_t>& pixels, int tile) {
    // Vibrant cyan / teal shirt
    fillNoise(pixels, tile, {0.0f, 0.66f, 0.66f}, 0.05f, 960);

    // V-neck skin collar at top center
    setPixel(pixels, tile, 7, 15, {0.78f, 0.56f, 0.41f});
    setPixel(pixels, tile, 8, 15, {0.78f, 0.56f, 0.41f});
    setPixel(pixels, tile, 7, 14, {0.78f, 0.56f, 0.41f});
    setPixel(pixels, tile, 8, 14, {0.78f, 0.56f, 0.41f});
    setPixel(pixels, tile, 6, 15, {0.74f, 0.52f, 0.38f});
    setPixel(pixels, tile, 9, 15, {0.74f, 0.52f, 0.38f});
}

void makePlayerArm(std::vector<uint8_t>& pixels, int tile) {
    // Bottom 2/3: skin tone
    fillNoise(pixels, tile, {0.78f, 0.56f, 0.41f}, 0.035f, 970);

    // Top 1/3: cyan shirt sleeve
    for (int y = 11; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, 971) - 0.5f) * 0.05f;
            setPixel(pixels, tile, x, y, {0.0f + n, 0.66f + n, 0.66f + n});
        }
    }
}

void makePlayerSkin(std::vector<uint8_t>& pixels, int tile) {
    fillNoise(pixels, tile, {0.78f, 0.56f, 0.41f}, 0.035f, 975);
}

void makePlayerPants(std::vector<uint8_t>& pixels, int tile) {
    // Dark indigo / blue denim pants
    fillNoise(pixels, tile, {0.23f, 0.20f, 0.49f}, 0.04f, 980);
}

void makePlayerShoe(std::vector<uint8_t>& pixels, int tile) {
    // Upper: blue pants
    fillNoise(pixels, tile, {0.23f, 0.20f, 0.49f}, 0.04f, 990);

    // Lower: gray shoes
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < TILE; ++x) {
            const float n = (rnd(x, y, 991) - 0.5f) * 0.04f;
            setPixel(pixels, tile, x, y, {0.40f + n, 0.40f + n, 0.40f + n});
        }
    }
}

void makeStick(std::vector<uint8_t>& pixels, int tile) {
    for (int i = 2; i <= 13; ++i) {
        setPixel(pixels, tile, i, i, {0.55f, 0.40f, 0.22f});
        if (i < 13) setPixel(pixels, tile, i, i + 1, {0.42f, 0.30f, 0.16f});
    }
}

void makeCoalItem(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 4; y <= 11; ++y) {
        for (int x = 4; x <= 11; ++x) {
            const int dx = x - 7;
            const int dy = y - 7;
            if (dx * dx + dy * dy <= 16) {
                const float n = (rnd(x, y, 77) - 0.5f) * 0.08f;
                const float c = 0.16f + n + (x == 6 && y >= 8 ? 0.15f : 0.0f);
                setPixel(pixels, tile, x, y, {c, c, c});
            }
        }
    }
}

void makeIronIngotItem(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 5; y <= 10; ++y) {
        const int inset = (y == 5 || y == 10) ? 1 : 0;
        for (int x = 3 + inset; x <= 12 - inset; ++x) {
            float bright = (y >= 9) ? 0.92f : ((y <= 6) ? 0.65f : 0.80f);
            bright += (rnd(x, y, 88) - 0.5f) * 0.05f;
            setPixel(pixels, tile, x, y, {bright, bright, bright * 1.05f});
        }
    }
}

void makeDiamondItem(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 3; y <= 12; ++y) {
        for (int x = 3; x <= 12; ++x) {
            const int dx = std::abs(x - 7);
            const int dy = std::abs(y - 7);
            if (dx + dy <= 5) {
                float r = 0.25f, g = 0.85f, b = 0.95f;
                if (y >= 8 && x <= 7) { r += 0.2f; g += 0.1f; }
                if (y <= 5) { r -= 0.1f; g -= 0.15f; b -= 0.1f; }
                setPixel(pixels, tile, x, y, {r, g, b});
            }
        }
    }
}

void makeTool(std::vector<uint8_t>& pixels, int tile, int toolType, Rgb head) {
    // Stick handle
    for (int i = 2; i <= 10; ++i) {
        setPixel(pixels, tile, i, i, {0.55f, 0.40f, 0.22f});
    }

    if (toolType == 0) { // Pickaxe
        const int headPts[][2] = {
            {6, 13}, {7, 13}, {8, 12}, {9, 12}, {10, 11}, {11, 10},
            {12, 9}, {12, 8}, {13, 7}, {13, 6}, {9, 9}, {10, 10}
        };
        for (const auto& pt : headPts) {
            const float n = (rnd(pt[0], pt[1], 101) - 0.5f) * 0.06f;
            setPixel(pixels, tile, pt[0], pt[1], {head.r + n, head.g + n, head.b + n});
        }
    } else if (toolType == 1) { // Axe
        for (int y = 8; y <= 13; ++y) {
            for (int x = 8; x <= 13; ++x) {
                if (x >= 9 && y >= 9 && (x >= 11 || y >= 11)) {
                    const float n = (rnd(x, y, 102) - 0.5f) * 0.06f;
                    setPixel(pixels, tile, x, y, {head.r + n, head.g + n, head.b + n});
                }
            }
        }
    } else if (toolType == 2) { // Shovel
        for (int y = 9; y <= 13; ++y) {
            for (int x = 9; x <= 13; ++x) {
                if (std::abs(x - 11) + std::abs(y - 11) <= 2) {
                    const float n = (rnd(x, y, 103) - 0.5f) * 0.06f;
                    setPixel(pixels, tile, x, y, {head.r + n, head.g + n, head.b + n});
                }
            }
        }
    } else if (toolType == 3) { // Sword
        // Guard
        setPixel(pixels, tile, 4, 6, {head.r * 0.8f, head.g * 0.8f, head.b * 0.8f});
        setPixel(pixels, tile, 5, 5, {head.r * 0.8f, head.g * 0.8f, head.b * 0.8f});
        setPixel(pixels, tile, 6, 4, {head.r * 0.8f, head.g * 0.8f, head.b * 0.8f});
        // Blade
        for (int i = 6; i <= 13; ++i) {
            const float n = (rnd(i, i, 104) - 0.5f) * 0.06f;
            setPixel(pixels, tile, i, i, {head.r + n, head.g + n, head.b + n});
            if (i <= 12) setPixel(pixels, tile, i, i + 1, {head.r * 0.9f + n, head.g * 0.9f + n, head.b * 0.9f + n});
        }
        setPixel(pixels, tile, 14, 14, {head.r * 1.1f, head.g * 1.1f, head.b * 1.1f});
    }
}

void makeFoodPorkchop(std::vector<uint8_t>& pixels, int tile, bool cooked) {
    // Porkchop shape with bone and meat
    for (int y = 3; y <= 12; ++y) {
        for (int x = 3; x <= 12; ++x) {
            const int dx = x - 8;
            const int dy = y - 7;
            if (dx * dx + dy * dy <= 18) {
                if (x <= 5 && y <= 5) {
                    // Bone end
                    setPixel(pixels, tile, x, y, {0.96f, 0.95f, 0.90f});
                } else if (cooked) {
                    // Roasted golden-brown porkchop with dark crust
                    const float edge = (dx * dx + dy * dy >= 14) ? 0.7f : 1.0f;
                    const float n = (rnd(x, y, 71) - 0.5f) * 0.08f;
                    setPixel(pixels, tile, x, y, {(0.78f + n) * edge, (0.45f + n) * edge, (0.22f + n) * edge});
                } else {
                    // Raw pink porkchop with fat rim
                    const bool isFat = (dx >= 2 || dy >= 2);
                    if (isFat) {
                        setPixel(pixels, tile, x, y, {0.96f, 0.82f, 0.85f});
                    } else {
                        const float n = (rnd(x, y, 72) - 0.5f) * 0.06f;
                        setPixel(pixels, tile, x, y, {0.92f + n, 0.50f + n, 0.55f + n});
                    }
                }
            }
        }
    }
}

void makeFoodBeef(std::vector<uint8_t>& pixels, int tile, bool cooked) {
    for (int y = 4; y <= 11; ++y) {
        for (int x = 3; x <= 13; ++x) {
            const int dx = x - 8;
            const int dy = y - 7;
            if (dx * dx * 1.2f + dy * dy * 1.8f <= 22.0f) {
                if (cooked) {
                    // Dark roasted steak with sear marks
                    const bool isGrillMark = ((x + y) % 3 == 0);
                    const float g = isGrillMark ? 0.65f : 1.0f;
                    const float n = (rnd(x, y, 73) - 0.5f) * 0.06f;
                    setPixel(pixels, tile, x, y, {(0.54f + n) * g, (0.28f + n) * g, (0.16f + n) * g});
                } else {
                    // Raw rich red steak with marbling
                    const bool isMarble = ((x * 2 + y * 3) % 5 == 0);
                    if (isMarble) {
                        setPixel(pixels, tile, x, y, {0.92f, 0.85f, 0.85f});
                    } else {
                        const float n = (rnd(x, y, 74) - 0.5f) * 0.06f;
                        setPixel(pixels, tile, x, y, {0.80f + n, 0.18f + n, 0.20f + n});
                    }
                }
            }
        }
    }
}

void makeApple(std::vector<uint8_t>& pixels, int tile) {
    // Apple fruit body
    for (int y = 3; y <= 11; ++y) {
        for (int x = 4; x <= 11; ++x) {
            const int dx = x - 7;
            const int dy = y - 7;
            if (dx * dx + dy * dy <= 16) {
                float r = 0.88f, g = 0.12f, b = 0.14f;
                // Highlight at top-left
                if (x == 6 && (y == 9 || y == 10)) {
                    r = 1.00f; g = 0.45f; b = 0.45f;
                } else if (y <= 4) {
                    r = 0.65f; g = 0.08f; b = 0.10f;
                }
                const float n = (rnd(x, y, 75) - 0.5f) * 0.05f;
                setPixel(pixels, tile, x, y, {r + n, g + n, b + n});
            }
        }
    }
    // Apple stem
    setPixel(pixels, tile, 7, 12, {0.45f, 0.28f, 0.12f});
    setPixel(pixels, tile, 8, 13, {0.45f, 0.28f, 0.12f});
    // Green leaf
    setPixel(pixels, tile, 9, 13, {0.25f, 0.78f, 0.20f});
    setPixel(pixels, tile, 10, 14, {0.20f, 0.65f, 0.15f});
}

void makeBread(std::vector<uint8_t>& pixels, int tile) {
    for (int y = 5; y <= 10; ++y) {
        for (int x = 3; x <= 12; ++x) {
            const int dx = x - 7;
            const int dy = y - 7;
            if (dx * dx * 0.9f + dy * dy * 2.2f <= 18.0f) {
                // Crust & score marks
                const bool isScore = (x == 5 && y >= 7) || (x == 8 && y >= 7) || (x == 11 && y >= 7);
                if (isScore) {
                    setPixel(pixels, tile, x, y, {0.96f, 0.88f, 0.65f});
                } else {
                    const float shadow = (y == 5) ? 0.7f : 1.0f;
                    const float n = (rnd(x, y, 76) - 0.5f) * 0.06f;
                    setPixel(pixels, tile, x, y, {(0.84f + n) * shadow, (0.58f + n) * shadow, (0.24f + n) * shadow});
                }
            }
        }
    }
}

void setPixelRgba(std::vector<uint8_t>& pixels, int tile, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= TILE || y < 0 || y >= TILE) return;
    const int tx = (tile % TILES) * TILE + x;
    const int ty = (tile / TILES) * TILE + y;
    const size_t i = (static_cast<size_t>(ty) * SIZE + tx) * 4;
    pixels[i + 0] = r;
    pixels[i + 1] = g;
    pixels[i + 2] = b;
    pixels[i + 3] = a;
}

void makeDestroyTile(std::vector<uint8_t>& pixels, int tile, int stage) {
    struct CrackSegment {
        int x0, y0, x1, y1;
        int minStage;
    };

    static const CrackSegment segments[] = {
        // Stage 0: Central initial fracture seeds
        {7, 8, 9, 8, 0}, {8, 7, 8, 10, 0},

        // Stage 1: Expanding central star fissures
        {9, 8, 11, 7, 1}, {7, 8, 5, 9, 1}, {8, 10, 7, 12, 1}, {8, 7, 9, 5, 1},

        // Stage 2: Extending toward quadrants
        {11, 7, 13, 5, 2}, {5, 9, 3, 11, 2}, {7, 12, 5, 13, 2}, {9, 5, 11, 3, 2},
        {8, 8, 10, 10, 2}, {7, 8, 6, 6, 2},

        // Stage 3: Diagonal forks & relief
        {10, 10, 13, 12, 3}, {6, 6, 4, 3, 3}, {13, 5, 14, 3, 3}, {3, 11, 2, 13, 3},
        {9, 8, 10, 6, 3}, {7, 8, 6, 10, 3},

        // Stage 4: Connecting to borders
        {13, 12, 15, 14, 4}, {4, 3, 2, 1, 4}, {14, 3, 15, 1, 4}, {2, 13, 1, 15, 4},
        {5, 9, 2, 8, 4}, {11, 7, 14, 8, 4},

        // Stage 5: Secondary cross-fissure web
        {8, 10, 11, 12, 5}, {8, 7, 5, 4, 5}, {6, 10, 4, 12, 5}, {10, 6, 12, 4, 5},
        {5, 13, 8, 15, 5}, {11, 3, 8, 1, 5},

        // Stage 6: Heavy intersecting mesh
        {4, 12, 2, 15, 6}, {12, 4, 15, 2, 6}, {11, 12, 14, 15, 6}, {5, 4, 1, 2, 6},
        {2, 8, 1, 5, 6}, {14, 8, 15, 11, 6}, {7, 12, 10, 14, 6}, {9, 5, 6, 2, 6},

        // Stage 7: Dense spiderweb fracturing
        {6, 6, 9, 6, 7}, {6, 10, 10, 10, 7}, {2, 13, 5, 15, 7}, {14, 3, 11, 1, 7},
        {4, 3, 7, 1, 7}, {12, 13, 15, 10, 7}, {3, 11, 1, 9, 7}, {13, 5, 15, 7, 7},

        // Stage 8: Deep severe fragmentation
        {2, 5, 4, 6, 8}, {14, 11, 12, 10, 8}, {7, 1, 9, 3, 8}, {9, 15, 7, 13, 8},
        {6, 2, 4, 4, 8}, {10, 14, 12, 12, 8}, {1, 9, 3, 7, 8}, {15, 7, 13, 9, 8},

        // Stage 9: Total catastrophic shattering
        {3, 7, 5, 9, 9}, {13, 9, 11, 7, 9}, {5, 4, 7, 6, 9}, {11, 12, 9, 10, 9},
        {1, 5, 1, 1, 9}, {15, 11, 15, 15, 9}, {5, 15, 1, 15, 9}, {11, 1, 15, 1, 9},
        {4, 12, 4, 15, 9}, {12, 4, 12, 1, 9}, {8, 8, 8, 4, 9}, {8, 8, 8, 12, 9}
    };

    for (const auto& cl : segments) {
        if (stage < cl.minStage) continue;

        int x0 = cl.x0, y0 = cl.y0, x1 = cl.x1, y1 = cl.y1;
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            // Authentic Minecraft crack: pitch black fissure core + stark white fracture highlight
            setPixelRgba(pixels, tile, x0, y0, 10, 10, 12, 255);
            setPixelRgba(pixels, tile, x0 + 1, y0, 245, 245, 252, 255);
            setPixelRgba(pixels, tile, x0, y0 - 1, 230, 230, 240, 255);

            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

void makeCraftingTable(std::vector<uint8_t>& pixels, int topTile, int sideTile, int frontTile) {
    // Base wood planks
    makePlanks(pixels, topTile);
    makePlanks(pixels, sideTile);
    makePlanks(pixels, frontTile);

    // Top: 2x2 grid inlay
    for (int y = 2; y <= 13; ++y) {
        for (int x = 2; x <= 13; ++x) {
            if (x == 7 || x == 8 || y == 7 || y == 8 || x == 2 || x == 13 || y == 2 || y == 13) {
                setPixel(pixels, topTile, x, y, {0.30f, 0.20f, 0.10f});
            } else {
                setPixel(pixels, topTile, x, y, {0.75f, 0.58f, 0.35f});
            }
        }
    }

    // Side & Front: saw / hammer tool silhouette
    for (int y = 4; y <= 11; ++y) {
        setPixel(pixels, sideTile, 4, y, {0.40f, 0.40f, 0.42f});
        setPixel(pixels, frontTile, 11, y, {0.40f, 0.40f, 0.42f});
    }
    setPixel(pixels, sideTile, 5, 11, {0.40f, 0.40f, 0.42f});
    setPixel(pixels, frontTile, 10, 11, {0.40f, 0.40f, 0.42f});
}

} // namespace

Texture::~Texture() {
    destroy();
}

void Texture::createAtlas() {
    std::vector<uint8_t> pixels(static_cast<size_t>(SIZE) * SIZE * 4, 0);

    const int grassTop = static_cast<int>(TextureTile::GrassTop);
    const int grassSide = static_cast<int>(TextureTile::GrassSide);
    const int dirt = static_cast<int>(TextureTile::Dirt);
    const int stone = static_cast<int>(TextureTile::Stone);
    const int woodSide = static_cast<int>(TextureTile::WoodSide);
    const int woodTop = static_cast<int>(TextureTile::WoodTop);
    const int leaves = static_cast<int>(TextureTile::Leaves);
    const int sand = static_cast<int>(TextureTile::Sand);
    const int bedrock = static_cast<int>(TextureTile::Bedrock);
    const int planks = static_cast<int>(TextureTile::Planks);
    const int water = static_cast<int>(TextureTile::Water);
    const int torch = static_cast<int>(TextureTile::Torch);
    const int cobblestone = static_cast<int>(TextureTile::Cobblestone);
    const int coalOre = static_cast<int>(TextureTile::CoalOre);
    const int ironOre = static_cast<int>(TextureTile::IronOre);
    const int goldOre = static_cast<int>(TextureTile::GoldOre);
    const int diamondOre = static_cast<int>(TextureTile::DiamondOre);
    const int tallGrass = static_cast<int>(TextureTile::TallGrass);
    const int dirtPathTop = static_cast<int>(TextureTile::DirtPathTop);
    const int dirtPathSide = static_cast<int>(TextureTile::DirtPathSide);
    const int pigSkin = static_cast<int>(TextureTile::PigSkin);
    const int pigFace = static_cast<int>(TextureTile::PigFace);
    const int pigSnout = static_cast<int>(TextureTile::PigSnout);
    const int cowSkin = static_cast<int>(TextureTile::CowSkin);
    const int cowFace = static_cast<int>(TextureTile::CowFace);
    const int cowHorns = static_cast<int>(TextureTile::CowHorns);
    const int pigmanFace = static_cast<int>(TextureTile::PigmanFace);
    const int pigmanSkin = static_cast<int>(TextureTile::PigmanSkin);
    const int pigmanTorso = static_cast<int>(TextureTile::PigmanTorso);
    const int pigmanHoof = static_cast<int>(TextureTile::PigmanHoof);
    const int playerFace = static_cast<int>(TextureTile::PlayerFace);
    const int playerHead = static_cast<int>(TextureTile::PlayerHead);
    const int playerTorso = static_cast<int>(TextureTile::PlayerTorso);
    const int playerArm = static_cast<int>(TextureTile::PlayerArm);
    const int playerSkin = static_cast<int>(TextureTile::PlayerSkin);
    const int playerPants = static_cast<int>(TextureTile::PlayerPants);
    const int playerShoe = static_cast<int>(TextureTile::PlayerShoe);
    const int stick = static_cast<int>(TextureTile::Stick);
    const int coal = static_cast<int>(TextureTile::Coal);
    const int ironIngot = static_cast<int>(TextureTile::IronIngot);
    const int diamond = static_cast<int>(TextureTile::Diamond);
    const int woodPickaxe = static_cast<int>(TextureTile::WoodPickaxe);
    const int stonePickaxe = static_cast<int>(TextureTile::StonePickaxe);
    const int ironPickaxe = static_cast<int>(TextureTile::IronPickaxe);
    const int diamondPickaxe = static_cast<int>(TextureTile::DiamondPickaxe);
    const int woodAxe = static_cast<int>(TextureTile::WoodAxe);
    const int stoneAxe = static_cast<int>(TextureTile::StoneAxe);
    const int ironAxe = static_cast<int>(TextureTile::IronAxe);
    const int diamondAxe = static_cast<int>(TextureTile::DiamondAxe);
    const int woodShovel = static_cast<int>(TextureTile::WoodShovel);
    const int stoneShovel = static_cast<int>(TextureTile::StoneShovel);
    const int ironShovel = static_cast<int>(TextureTile::IronShovel);
    const int diamondShovel = static_cast<int>(TextureTile::DiamondShovel);
    const int woodSword = static_cast<int>(TextureTile::WoodSword);
    const int stoneSword = static_cast<int>(TextureTile::StoneSword);
    const int ironSword = static_cast<int>(TextureTile::IronSword);
    const int diamondSword = static_cast<int>(TextureTile::DiamondSword);
    const int craftingTableTop = static_cast<int>(TextureTile::CraftingTableTop);
    const int craftingTableSide = static_cast<int>(TextureTile::CraftingTableSide);
    const int craftingTableFront = static_cast<int>(TextureTile::CraftingTableFront);
    const int rawPorkchop = static_cast<int>(TextureTile::RawPorkchop);
    const int cookedPorkchop = static_cast<int>(TextureTile::CookedPorkchop);
    const int rawBeef = static_cast<int>(TextureTile::RawBeef);
    const int cookedBeef = static_cast<int>(TextureTile::CookedBeef);
    const int apple = static_cast<int>(TextureTile::Apple);
    const int bread = static_cast<int>(TextureTile::Bread);
    const int destroy0 = static_cast<int>(TextureTile::Destroy0);

    const Rgb woodHead = {0.60f, 0.45f, 0.25f};
    const Rgb stoneHead = {0.55f, 0.55f, 0.55f};
    const Rgb ironHead = {0.85f, 0.85f, 0.88f};
    const Rgb diamondHead = {0.35f, 0.88f, 0.95f};

    makeStick(pixels, stick);
    makeCoalItem(pixels, coal);
    makeIronIngotItem(pixels, ironIngot);
    makeDiamondItem(pixels, diamond);
    makeTool(pixels, woodPickaxe, 0, woodHead);
    makeTool(pixels, stonePickaxe, 0, stoneHead);
    makeTool(pixels, ironPickaxe, 0, ironHead);
    makeTool(pixels, diamondPickaxe, 0, diamondHead);
    makeTool(pixels, woodAxe, 1, woodHead);
    makeTool(pixels, stoneAxe, 1, stoneHead);
    makeTool(pixels, ironAxe, 1, ironHead);
    makeTool(pixels, diamondAxe, 1, diamondHead);
    makeTool(pixels, woodShovel, 2, woodHead);
    makeTool(pixels, stoneShovel, 2, stoneHead);
    makeTool(pixels, ironShovel, 2, ironHead);
    makeTool(pixels, diamondShovel, 2, diamondHead);
    makeTool(pixels, woodSword, 3, woodHead);
    makeTool(pixels, stoneSword, 3, stoneHead);
    makeTool(pixels, ironSword, 3, ironHead);
    makeTool(pixels, diamondSword, 3, diamondHead);
    makeCraftingTable(pixels, craftingTableTop, craftingTableSide, craftingTableFront);
    makeFoodPorkchop(pixels, rawPorkchop, false);
    makeFoodPorkchop(pixels, cookedPorkchop, true);
    makeFoodBeef(pixels, rawBeef, false);
    makeFoodBeef(pixels, cookedBeef, true);
    makeApple(pixels, apple);
    makeBread(pixels, bread);

    for (int s = 0; s < 10; ++s) {
        if (!loadTilePng(pixels, destroy0 + s, "destroy_stage_" + std::to_string(s) + ".png")) {
            makeDestroyTile(pixels, destroy0 + s, s);
        }
    }


    const Rgb grassTint = {0.45f, 0.76f, 0.26f};
    const Rgb leavesTint = {0.32f, 0.68f, 0.22f};

    if (!loadTilePng(pixels, grassTop, "grass_block_top.png", grassTint)) fillNoise(pixels, grassTop, {0.37f, 0.62f, 0.21f}, 0.06f, 5);
    if (!loadTilePng(pixels, grassSide, "grass_block_side.png"))           makeGrassSide(pixels, grassSide);
    if (!loadTilePng(pixels, dirt, "dirt.png"))                            fillNoise(pixels, dirt, {0.52f, 0.37f, 0.26f}, 0.06f, 9);
    if (!loadTilePng(pixels, stone, "stone.png"))                          makeStone(pixels, stone);
    if (!loadTilePng(pixels, woodSide, "oak_log.png"))                     makeWoodSide(pixels, woodSide);
    if (!loadTilePng(pixels, woodTop, "oak_log_top.png"))                  makeWoodTop(pixels, woodTop);
    if (!loadTilePng(pixels, leaves, "oak_leaves.png", leavesTint))        makeLeaves(pixels, leaves);
    if (!loadTilePng(pixels, sand, "sand.png"))                            fillNoise(pixels, sand, {0.86f, 0.81f, 0.64f}, 0.04f, 23);
    if (!loadTilePng(pixels, bedrock, "bedrock.png"))                      makeBedrock(pixels, bedrock);
    if (!loadTilePng(pixels, planks, "oak_planks.png"))                    makePlanks(pixels, planks);
    if (!loadTilePng(pixels, water, "water.png"))                          makeWater(pixels, water);
    if (!loadTilePng(pixels, torch, "torch.png"))                          makeTorch(pixels, torch);
    if (!loadTilePng(pixels, cobblestone, "cobblestone.png"))              makeCobblestone(pixels, cobblestone);
    if (!loadTilePng(pixels, coalOre, "coal_ore.png"))                     makeOre(pixels, coalOre, {0.15f, 0.15f, 0.15f}, 111);
    if (!loadTilePng(pixels, ironOre, "iron_ore.png"))                     makeOre(pixels, ironOre, {0.82f, 0.68f, 0.58f}, 222);
    if (!loadTilePng(pixels, goldOre, "gold_ore.png"))                     makeOre(pixels, goldOre, {0.98f, 0.88f, 0.25f}, 333);
    if (!loadTilePng(pixels, diamondOre, "diamond_ore.png"))               makeOre(pixels, diamondOre, {0.35f, 0.95f, 0.95f}, 444);
    if (!loadTilePng(pixels, tallGrass, "tall_grass.png", grassTint) &&
        !loadTilePng(pixels, tallGrass, "short_grass.png", grassTint))     makeTallGrass(pixels, tallGrass);
    if (!loadTilePng(pixels, dirtPathTop, "dirt_path_top.png") &&
        !loadTilePng(pixels, dirtPathTop, "grass_path_top.png"))           makeDirtPathTop(pixels, dirtPathTop);
    if (!loadTilePng(pixels, dirtPathSide, "dirt_path_side.png") &&
        !loadTilePng(pixels, dirtPathSide, "grass_path_side.png"))         makeDirtPathSide(pixels, dirtPathSide);

    makePigSkin(pixels, pigSkin);
    makePigFace(pixels, pigFace);
    makePigSnout(pixels, pigSnout);
    makeCowSkin(pixels, cowSkin);
    makeCowFace(pixels, cowFace);
    makeCowHorns(pixels, cowHorns);
    makePigmanFace(pixels, pigmanFace);
    makePigmanSkin(pixels, pigmanSkin);
    makePigmanTorso(pixels, pigmanTorso);
    makePigmanHoof(pixels, pigmanHoof);
    makePlayerFace(pixels, playerFace);
    makePlayerHead(pixels, playerHead);
    makePlayerTorso(pixels, playerTorso);
    makePlayerArm(pixels, playerArm);
    makePlayerSkin(pixels, playerSkin);
    makePlayerPants(pixels, playerPants);
    makePlayerShoe(pixels, playerShoe);

    const int maxLod = static_cast<int>(std::floor(std::log2(static_cast<double>(TILE))));

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLod);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    log::info("Texture atlas generated (%dx%d, %dx%d per tile, max mip level %d)", SIZE, SIZE, TILE, TILE, maxLod);
}

void Texture::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::destroy() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

} // namespace vox
