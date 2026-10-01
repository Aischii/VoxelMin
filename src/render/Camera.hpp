#pragma once
#include <glm/glm.hpp>

namespace vox {

// First-person camera defined by position + yaw/pitch (degrees).
//   yaw   : -90 looks down -Z
//   pitch : clamped to (-89, 89) to avoid gimbal issues
class Camera {
public:
    void setPosition(const glm::vec3& p) { m_position = p; }
    const glm::vec3& position() const { return m_position; }

    void setRotation(float yaw, float pitch, float roll = 0.0f) {
        m_yaw = yaw;
        m_pitch = pitch;
        m_roll = roll;
    }
    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float roll() const { return m_roll; }

    void setFov(float fov) { m_fov = fov; }
    float fov() const { return m_fov; }

    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::vec3 up() const { return glm::normalize(glm::cross(right(), front())); }

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

private:
    glm::vec3 m_position{0.0f};
    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_roll = 0.0f;
    float m_fov = 70.0f;
};

} // namespace vox
