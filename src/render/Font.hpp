#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vox {

// ---------------------------------------------------------------------------
// Bitmap / TTF / OTF Font Renderer
// Supports loading TrueType / OpenType pixel fonts (such as Born2bSporty)
// via stb_truetype with automatic fallback to the built-in procedural glyph table.
// ---------------------------------------------------------------------------
class Font {
public:
    static constexpr int GlyphWidth  = 5;
    static constexpr int GlyphHeight = 7;
    static constexpr int CellWidth   = 6;
    static constexpr int CellHeight  = 8;

    struct Glyph {
        float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        float xadvance = 6.0f;
    };

    bool build(const std::string& fontPath = "");
    void destroy();
    void bind(uint32_t unit = 0) const;

    const Glyph* glyph(char c) const;
    float textWidth(const std::string& text, float scale) const;
    float textHeight(float scale) const { return m_fontHeight * scale; }

    uint32_t id() const { return m_texture; }

private:
    bool buildFromTtf(const std::string& fontPath);
    bool buildFallback();

    uint32_t m_texture = 0;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    float m_fontHeight = 8.0f;
    std::array<int, 256> m_index{};
    std::vector<Glyph> m_glyphs;
};

} // namespace vox
