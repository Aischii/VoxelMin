#pragma once
#include <functional>
#include <string>
#include <vector>

namespace vox {

class Renderer;

// A vertical list of clickable/keyboard-navigable rows, rendered in a
// Legacy-console-inspired style (dark translucent rows, gold highlight, small
// selection marker). Rows are either buttons (action) or options (value +
// adjust callback driven by Left/Right).
class Menu {
public:
    struct Row {
        std::string label;
        std::function<void()> action;       // buttons
        std::function<std::string()> value; // options (non-null => option row)
        std::function<void(int)> adjust;    // options: dir = -1 / +1
        bool enabled = true;

        // Layout, filled in by render() and used by rowAt().
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        bool isOption() const { return static_cast<bool>(value); }
    };

    Menu& title(const std::string& value) { m_title = value; return *this; }
    Menu& subtitle(const std::string& value) { m_subtitle = value; return *this; }
    Menu& splash(const std::string& value) { m_splash = value; return *this; }
    Menu& footer(const std::string& value) { m_footer = value; return *this; }
    Menu& logo(bool value) { m_isLogo = value; return *this; }
    Menu& backAction(std::function<void()> action) { m_back = std::move(action); return *this; }

    void clear();
    void addButton(const std::string& label, std::function<void()> action);
    void addOption(const std::string& label,
                   std::function<std::string()> value,
                   std::function<void(int)> adjust);

    void resetSelection();
    void moveSelection(int delta);
    void activate();
    void adjustSelected(int dir);
    void setSelection(int index);
    int selection() const { return m_selection; }
    const Row* getRow(int index) const {
        if (index >= 0 && index < static_cast<int>(m_rows.size())) return &m_rows[static_cast<size_t>(index)];
        return nullptr;
    }

    // Row index under a screen-pixel position, or -1.
    int rowAt(float px, float py) const;

    void render(Renderer& renderer, double uiTime);

private:
    std::vector<Row> m_rows;
    int m_selection = 0;
    std::string m_title;
    std::string m_subtitle;
    std::string m_splash;
    std::string m_footer;
    bool m_isLogo = false;
    std::function<void()> m_back;
};

} // namespace vox
