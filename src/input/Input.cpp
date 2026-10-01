#include "input/Input.hpp"

namespace vox {

void Input::newFrame() {
    m_keyPressed.fill(false);
    m_mousePressed.fill(false);
    m_dx = 0.0;
    m_dy = 0.0;
    m_scrollY = 0.0;
}

void Input::onKey(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if (key < 0 || key > GLFW_KEY_LAST) return;

    if (action == GLFW_PRESS) {
        m_keyPressed[key] = !m_keyDown[key];
        m_keyDown[key] = true;
    } else if (action == GLFW_RELEASE) {
        m_keyDown[key] = false;
    }
}

void Input::onMouseButton(int button, int action, int mods) {
    (void)mods;
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return;

    if (action == GLFW_PRESS) {
        m_mousePressed[button] = !m_mouseDown[button];
        m_mouseDown[button] = true;
    } else if (action == GLFW_RELEASE) {
        m_mouseDown[button] = false;
    }
}

void Input::onCursorPos(double x, double y) {
    if (m_firstMouse) {
        m_lastX = x;
        m_lastY = y;
        m_firstMouse = false;
        return;
    }
    m_dx += x - m_lastX;
    m_dy += y - m_lastY;
    m_lastX = x;
    m_lastY = y;
}

void Input::onScroll(double xOffset, double yOffset) {
    (void)xOffset;
    m_scrollY += yOffset;
}

bool Input::keyDown(int key) const {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return m_keyDown[key];
}

bool Input::keyPressed(int key) const {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return m_keyPressed[key];
}

bool Input::mouseDown(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return m_mouseDown[button];
}

bool Input::mousePressed(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return m_mousePressed[button];
}

} // namespace vox
