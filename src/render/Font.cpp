#include "render/Font.hpp"
#include "core/Log.hpp"
#include "render/FontData.hpp"

#include <GL/glew.h>

#include <cstring>

namespace vox {

bool Font::build() {
    m_index.fill(-1);

    const int count = static_cast<int>(sizeof(RAW_GLYPHS) / sizeof(RAW_GLYPHS[0]));
    const int columns = 16;
    const int rows = (count + columns - 1) / columns;

    m_atlasWidth = columns * CellWidth;
    m_atlasHeight = rows * CellHeight;
    m_glyphs.assign(static_cast<size_t>(count), Glyph{});
    std::vector<uint8_t> pixels(static_cast<size_t>(m_atlasWidth) * m_atlasHeight, 0);

    for (int i = 0; i < count; ++i) {
        const RawGlyph& raw = RAW_GLYPHS[i];
        const unsigned char code = static_cast<unsigned char>(raw.ch);
        if (code < 128) m_index[code] = i;

        const int cellX = (i % columns) * CellWidth;
        const int cellY = (i / columns) * CellHeight;

        m_glyphs[i].u0 = static_cast<float>(cellX) / m_atlasWidth;
        m_glyphs[i].v0 = static_cast<float>(cellY) / m_atlasHeight;
        m_glyphs[i].u1 = static_cast<float>(cellX + GlyphWidth) / m_atlasWidth;
        m_glyphs[i].v1 = static_cast<float>(cellY + GlyphHeight) / m_atlasHeight;

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

    log::info("Font atlas built: %d glyphs (%dx%d)", count, m_atlasWidth, m_atlasHeight);
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
    if (code >= 128) return nullptr;
    const int index = m_index[code];
    if (index < 0) return nullptr;
    return &m_glyphs[static_cast<size_t>(index)];
}

float Font::textWidth(const std::string& text, float scale) const {
    return static_cast<float>(text.size()) * CellWidth * scale;
}

} // namespace vox
