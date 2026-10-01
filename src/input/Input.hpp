#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <array>

namespace vox {

// Polled input state. GLFW callbacks feed this object (see Application), and
// gameplay code reads it once per frame.
//
// Edge helpers (keyPressed / mousePressed) are true only on the frame the
// button transitioned from up to down. Call newFrame() once per frame, before
// glfwPollEvents(), to clear those edges.
class Input {
public:
    void newFrame();

    void onKey(int key, int scancode, int action, int mods);
    void onMouseButton(int button, int action, int mods);
    void onCursorPos(double x, double y);
    void onScroll(double xOffset, double yOffset);

    bool keyDown(int key) const;
    bool keyPressed(int key) const;
    bool mouseDown(int button) const;
    bool mousePressed(int button) const;

    float mouseDeltaX() const { return static_cast<float>(m_dx); }
    float mouseDeltaY() const { return static_cast<float>(m_dy); }
    double scrollY() const { return m_scrollY; }

    bool cursorCaptured() const { return m_captured; }
    void setCursorCaptured(bool captured) { m_captured = captured; }

    // Call when re-capturing the cursor so the first reported position does not
    // generate a huge look delta.
    void resetMouseTracking() {
        m_firstMouse = true;
        m_dx = 0.0;
        m_dy = 0.0;
    }

private:
    std::array<bool, GLFW_KEY_LAST + 1> m_keyDown{};
    std::array<bool, GLFW_KEY_LAST + 1> m_keyPressed{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseDown{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_mousePressed{};

    double m_lastX = 0.0, m_lastY = 0.0;
    double m_dx = 0.0, m_dy = 0.0;
    double m_scrollY = 0.0;
    bool m_firstMouse = true;
    bool m_captured = false;
};

} // namespace vox
