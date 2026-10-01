#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vox {

// Bitmap font built at startup from the data in FontData.hpp. Glyphs live in a
// single-channel OpenGL texture (one atlas arranged in a grid of 6x8 cells).
class Font {
public:
    static constexpr int GlyphWidth  = 5;
    static constexpr int GlyphHeight = 7;
    static constexpr int CellWidth   = 6; // glyph + 1px spacing
    static constexpr int CellHeight  = 8;

    struct Glyph {
        float u0, v0, u1, v1;
    };

    bool build();
    void destroy();
    void bind(uint32_t unit = 0) const;

    const Glyph* glyph(char c) const;
    float textWidth(const std::string& text, float scale) const;
    float textHeight(float scale) const { return GlyphHeight * scale; }

    uint32_t id() const { return m_texture; }

private:
    uint32_t m_texture = 0;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    std::array<int, 128> m_index{}; // ASCII -> atlas index (-1 = missing)
    std::vector<Glyph> m_glyphs;
};

} // namespace vox
