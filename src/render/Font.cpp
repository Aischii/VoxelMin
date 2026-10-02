#include "render/Font.hpp"
#include "core/Log.hpp"
#include "render/FontData.hpp"

#include <GL/glew.h>

#define STB_TRUETYPE_IMPLEMENTATION
#if __has_include(<stb/stb_truetype.h>)
#include <stb/stb_truetype.h>
#elif __has_include(<stb_truetype.h>)
#include <stb_truetype.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

namespace vox {

bool Font::build(const std::string& fontPath) {
    if (!fontPath.empty() && buildFromTtf(fontPath)) {
        return true;
    }
    return buildFallback();
}

bool Font::buildFromTtf(const std::string& fontPath) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        log::warn("Font file '%s' could not be opened, using fallback font", fontPath.c_str());
        return false;
    }

    const std::streamsize size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> ttfBuffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size)) {
        return false;
    }

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttfBuffer.data(), 0)) {
        log::warn("stbtt_InitFont failed for '%s'", fontPath.c_str());
        return false;
    }

    m_atlasWidth = 256;
    m_atlasHeight = 256;
    m_fontHeight = 14.0f; // Normalized line height

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    const float fontScale = stbtt_ScaleForPixelHeight(&info, m_fontHeight);
    const float fontAscent = static_cast<float>(ascent) * fontScale;

    std::vector<uint8_t> bitmap(static_cast<size_t>(m_atlasWidth) * m_atlasHeight, 0);
    stbtt_bakedchar chardata[96]; // ASCII 32..127

    const int res = stbtt_BakeFontBitmap(ttfBuffer.data(), 0, m_fontHeight,
                                         bitmap.data(), m_atlasWidth, m_atlasHeight,
                                         32, 96, chardata);
    if (res <= 0) {
        log::warn("stbtt_BakeFontBitmap failed for '%s'", fontPath.c_str());
        return false;
    }

    m_index.fill(-1);
    m_glyphs.assign(96, Glyph{});

    const float invW = 1.0f / static_cast<float>(m_atlasWidth);
    const float invH = 1.0f / static_cast<float>(m_atlasHeight);

    for (int i = 0; i < 96; ++i) {
        const unsigned char c = static_cast<unsigned char>(32 + i);
        m_index[c] = i;

        const stbtt_bakedchar& bc = chardata[i];
        m_glyphs[i].u0 = static_cast<float>(bc.x0) * invW;
        m_glyphs[i].v0 = static_cast<float>(bc.y0) * invH;
        m_glyphs[i].u1 = static_cast<float>(bc.x1) * invW;
        m_glyphs[i].v1 = static_cast<float>(bc.y1) * invH;

        const float gw = static_cast<float>(bc.x1 - bc.x0);
        const float gh = static_cast<float>(bc.y1 - bc.y0);

        m_glyphs[i].x0 = bc.xoff;
        m_glyphs[i].y0 = -(fontAscent + bc.yoff);
        m_glyphs[i].x1 = bc.xoff + gw;
        m_glyphs[i].y1 = -(fontAscent + bc.yoff) - gh;
        m_glyphs[i].xadvance = bc.xadvance;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_atlasWidth, m_atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    log::info("Loaded TrueType/OpenType font: '%s' (%dx%d atlas, size %.1fpx)",
              fontPath.c_str(), m_atlasWidth, m_atlasHeight, m_fontHeight);
    return true;
}

bool Font::buildFallback() {
    m_index.fill(-1);
    const int count = static_cast<int>(sizeof(RAW_GLYPHS) / sizeof(RAW_GLYPHS[0]));
    const int columns = 16;
    const int rows = (count + columns - 1) / columns;

    m_atlasWidth = columns * CellWidth;
    m_atlasHeight = rows * CellHeight;
    m_fontHeight = static_cast<float>(GlyphHeight);
    m_glyphs.assign(static_cast<size_t>(count), Glyph{});
    std::vector<uint8_t> pixels(static_cast<size_t>(m_atlasWidth) * m_atlasHeight, 0);

    for (int i = 0; i < count; ++i) {
        const RawGlyph& raw = RAW_GLYPHS[i];
        const unsigned char code = static_cast<unsigned char>(raw.ch);
        m_index[code] = i;

        const int cellX = (i % columns) * CellWidth;
        const int cellY = (i / columns) * CellHeight;

        m_glyphs[i].u0 = static_cast<float>(cellX) / m_atlasWidth;
        m_glyphs[i].v0 = static_cast<float>(cellY) / m_atlasHeight;
        m_glyphs[i].u1 = static_cast<float>(cellX + GlyphWidth) / m_atlasWidth;
        m_glyphs[i].v1 = static_cast<float>(cellY + GlyphHeight) / m_atlasHeight;

        m_glyphs[i].x0 = 0.0f;
        m_glyphs[i].y0 = 0.0f;
        m_glyphs[i].x1 = static_cast<float>(GlyphWidth);
        m_glyphs[i].y1 = -static_cast<float>(GlyphHeight);
        m_glyphs[i].xadvance = static_cast<float>(CellWidth);

        for (int gy = 0; gy < GlyphHeight; ++gy) {
            const char* row = raw.rows[gy];
            const int rowLength = static_cast<int>(std::strlen(row));
            for (int gx = 0; gx < GlyphWidth; ++gx) {
                const char c = (gx < rowLength) ? row[gx] : '.';
                if (c != '#') continue;
                const int px = cellX + gx;
                const int py = cellY + gy;
                pixels[static_cast<size_t>(py) * m_atlasWidth + px] = 255;
            }
        }
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_atlasWidth, m_atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    log::info("Font fallback built: %d glyphs (%dx%d)", count, m_atlasWidth, m_atlasHeight);
    return true;
}

void Font::destroy() {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
}

void Font::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

const Font::Glyph* Font::glyph(char c) const {
    const unsigned char code = static_cast<unsigned char>(c);
    const int idx = m_index[code];
    if (idx < 0 || static_cast<size_t>(idx) >= m_glyphs.size()) return nullptr;
    return &m_glyphs[static_cast<size_t>(idx)];
}

float Font::textWidth(const std::string& text, float scale) const {
    float width = 0.0f;
    for (char c : text) {
        const Glyph* g = glyph(c);
        if (g) {
            width += g->xadvance * scale;
        } else {
            width += CellWidth * scale;
        }
    }
    return width;
}

} // namespace vox
