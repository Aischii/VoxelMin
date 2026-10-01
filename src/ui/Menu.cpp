#include "ui/Menu.hpp"
#include "render/Renderer.hpp"

#include <algorithm>
#include <cmath>

namespace vox {
namespace {

glm::vec4 rgba(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

} // namespace

void Menu::clear() {
    m_rows.clear();
    m_selection = 0;
    m_back = nullptr;
}

void Menu::addButton(const std::string& label, std::function<void()> action) {
    Row row;
    row.label = label;
    row.action = std::move(action);
    m_rows.push_back(std::move(row));
}

void Menu::addOption(const std::string& label,
                     std::function<std::string()> value,
                     std::function<void(int)> adjust) {
    Row row;
    row.label = label;
    row.value = std::move(value);
    row.adjust = std::move(adjust);
    m_rows.push_back(std::move(row));
}

void Menu::resetSelection() {
    m_selection = 0;
    if (!m_rows.empty() && !m_rows[0].enabled) moveSelection(1);
}

void Menu::setSelection(int index) {
    if (index < 0 || index >= static_cast<int>(m_rows.size())) return;
    if (!m_rows[static_cast<size_t>(index)].enabled) return;
    m_selection = index;
}

void Menu::moveSelection(int delta) {
    const int count = static_cast<int>(m_rows.size());
    if (count == 0) return;
    int index = m_selection;
    for (int step = 0; step < count; ++step) {
        index = (index + delta + count) % count;
        if (m_rows[static_cast<size_t>(index)].enabled) {
            m_selection = index;
            return;
        }
    }
}

void Menu::activate() {
    if (m_selection < 0 || m_selection >= static_cast<int>(m_rows.size())) return;
    Row& row = m_rows[static_cast<size_t>(m_selection)];
    if (!row.enabled) return;
    if (row.action) {
        row.action();
    } else if (row.adjust) {
        row.adjust(1);
    }
}

void Menu::adjustSelected(int dir) {
    if (m_selection < 0 || m_selection >= static_cast<int>(m_rows.size())) return;
    Row& row = m_rows[static_cast<size_t>(m_selection)];
    if (row.enabled && row.adjust) row.adjust(dir);
}

int Menu::rowAt(float px, float py) const {
    for (size_t i = 0; i < m_rows.size(); ++i) {
        const Row& row = m_rows[i];
        if (px >= row.x && px <= row.x + row.w && py >= row.y && py <= row.y + row.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Menu::render(Renderer& renderer, double uiTime) {
    const float width = static_cast<float>(renderer.width());
    const float height = static_cast<float>(renderer.height());

    // GUI scale is owned by the Application (see the GUI Scale option) and read
    // back here so the HUD and menus always agree.
    const float uiScale = renderer.uiScale();
    const float textScale = uiScale;

    // Footer kept clear of the bottom edge so it never collides with the
    // taskbar in fullscreen / borderless windows.
    const float footerScale = std::max(1.0f, uiScale - 1.0f);
    const float footerGlyph = renderer.textHeight(footerScale);
    const float footerBottom = std::max(22.0f, height * 0.04f);
    const float footerTop = footerBottom + footerGlyph;

    renderer.beginUI();

    // --- Title / logo -------------------------------------------------------
    const std::string title = m_title;
    if (m_isLogo) {
        const float titleScale = uiScale * 2.0f;
        const float titleW = renderer.textWidth(title, titleScale);
        const float tx = (width - titleW) * 0.5f;
        const float ty = height * 0.88f;

        renderer.drawText(tx + 4.0f, ty - 4.0f, title, titleScale, rgba(0.10f, 0.10f, 0.10f, 0.85f));
        renderer.drawText(tx, ty, title, titleScale, rgba(0.86f, 0.88f, 0.86f, 1.0f));
        // Underline accent.
        renderer.drawRect(tx, ty - renderer.textHeight(titleScale) - 6.0f * uiScale,
                          titleW, 3.0f * uiScale, rgba(0.42f, 0.68f, 0.30f, 0.95f));

        if (!m_subtitle.empty()) {
            const float subScale = std::max(1.0f, uiScale - 1.0f);
            const float subW = renderer.textWidth(m_subtitle, subScale);
            renderer.drawText((width - subW) * 0.5f,
                              ty - renderer.textHeight(titleScale) - 18.0f * uiScale,
                              m_subtitle, subScale, rgba(0.95f, 0.95f, 0.95f, 0.85f));
        }

        if (!m_splash.empty()) {
            const float pulse = 1.0f + 0.06f * static_cast<float>(std::sin(uiTime * 3.0));
            const float splashScale = (uiScale - 0.5f) * pulse;
            renderer.drawText(tx + titleW + 20.0f * uiScale, ty - 6.0f * uiScale,
                              m_splash, splashScale, rgba(1.0f, 1.0f, 0.20f, 1.0f),
                              -0.26f);
        }
    } else if (!title.empty()) {
        const float titleScale = std::max(1.0f, uiScale * 1.25f);
        const float titleW = renderer.textWidth(title, titleScale);
        const float tx = (width - titleW) * 0.5f;
        const float ty = height * 0.86f;
        renderer.drawText(tx + 3.0f, ty - 3.0f, title, titleScale, rgba(0.0f, 0.0f, 0.0f, 0.6f));
        renderer.drawText(tx, ty, title, titleScale, rgba(0.96f, 0.96f, 0.96f, 1.0f));
        if (!m_subtitle.empty()) {
            const float subScale = std::max(1.0f, uiScale - 1.0f);
            const float subW = renderer.textWidth(m_subtitle, subScale);
            renderer.drawText((width - subW) * 0.5f,
                              ty - renderer.textHeight(titleScale) - 10.0f * uiScale,
                              m_subtitle, subScale, rgba(0.9f, 0.9f, 0.9f, 0.8f));
        }
    }

    // --- Buttons ------------------------------------------------------------
    const float buttonW = std::min(170.0f * uiScale, width * 0.6f);
    const float buttonH = 14.0f * uiScale;
    const float gap = 4.0f * uiScale;
    const float count = static_cast<float>(m_rows.size());
    const float totalH = count * buttonH + std::max(0.0f, count - 1.0f) * gap;

    // Never let the lowest button overlap the footer; if the stack is taller
    // than the space below the title, shift it up (it may approach the title
    // at extreme GUI scales, but stays fully on screen).
    const float regionBottom = footerTop + 8.0f * uiScale;
    float top = height * 0.62f;
    if (top - totalH < regionBottom) top = totalH + regionBottom;
    const float left = (width - buttonW) * 0.5f;

    const float glyphH = renderer.textHeight(textScale);
    const float pad = 5.0f * uiScale;

    for (size_t i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];
        const bool selected = (static_cast<int>(i) == m_selection);
        const float rowTop = top - static_cast<float>(i) * (buttonH + gap);
        row.x = left;
        row.y = rowTop - buttonH;
        row.w = buttonW;
        row.h = buttonH;

        // Selection outline + left marker.
        if (selected) {
            renderer.drawRect(row.x - 3.0f, row.y - 3.0f, row.w + 6.0f, row.h + 6.0f,
                              rgba(0.98f, 0.80f, 0.25f, 1.0f));
            const float midY = row.y + row.h * 0.5f;
            const float m = 5.0f * uiScale;
            renderer.drawTriangle({row.x - 3.0f - m, midY + m},
                                  {row.x - 3.0f - m, midY - m},
                                  {row.x - 4.0f, midY},
                                  rgba(0.98f, 0.80f, 0.25f, 1.0f));
        }

        // Body: dark border + gradient fill + top highlight.
        renderer.drawRect(row.x, row.y, row.w, row.h, rgba(0.05f, 0.05f, 0.06f, 0.95f));
        const glm::vec4 fillBottom = selected ? rgba(0.84f, 0.64f, 0.18f, 1.0f)
                                              : rgba(0.17f, 0.17f, 0.19f, 0.95f);
        const glm::vec4 fillTop = selected ? rgba(0.99f, 0.88f, 0.50f, 1.0f)
                                           : rgba(0.33f, 0.33f, 0.36f, 0.95f);
        const float inset = std::max(1.0f, uiScale * 0.66f);
        renderer.drawGradientRect(row.x + inset, row.y + inset,
                                  row.w - 2.0f * inset, row.h - 2.0f * inset,
                                  fillBottom, fillTop);
        renderer.drawRect(row.x + inset, row.y + row.h - inset - inset * 0.66f,
                          row.w - 2.0f * inset, inset * 0.66f,
                          selected ? rgba(1.0f, 1.0f, 1.0f, 0.35f) : rgba(1.0f, 1.0f, 1.0f, 0.10f));

        const glm::vec4 textColor = selected ? rgba(0.12f, 0.09f, 0.02f, 1.0f)
                                             : rgba(0.93f, 0.93f, 0.93f, 1.0f);
        const float textTop = row.y + row.h * 0.5f + glyphH * 0.5f;

        if (row.isOption()) {
            renderer.drawText(row.x + pad + 2.0f, textTop, row.label, textScale, textColor);
            const std::string rawValue = row.value ? row.value() : std::string();
            const std::string valueStr = selected ? ("< " + rawValue + " >") : rawValue;
            const float valueW = renderer.textWidth(valueStr, textScale);
            renderer.drawText(row.x + row.w - pad - 2.0f - valueW, textTop, valueStr, textScale, textColor);
        } else {
            const float labelW = renderer.textWidth(row.label, textScale);
            renderer.drawText(row.x + (row.w - labelW) * 0.5f, textTop, row.label, textScale, textColor);
        }
    }

    // --- Footer -------------------------------------------------------------
    if (!m_footer.empty()) {
        const float footerW = renderer.textWidth(m_footer, footerScale);
        renderer.drawText((width - footerW) * 0.5f + 1.0f, footerTop - 1.0f, m_footer,
                          footerScale, rgba(0.0f, 0.0f, 0.0f, 0.5f));
        renderer.drawText((width - footerW) * 0.5f, footerTop, m_footer,
                          footerScale, rgba(0.92f, 0.92f, 0.92f, 0.8f));
    }

    renderer.endUI();
}

} // namespace vox
