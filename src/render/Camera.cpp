#include "render/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace vox {

glm::vec3 Camera::front() const {
    const float yawRad = glm::radians(m_yaw);
    const float pitchRad = glm::radians(m_pitch);
    const float cp = std::cos(pitchRad);
    return glm::normalize(glm::vec3(std::cos(yawRad) * cp,
                                    std::sin(pitchRad),
                                    std::sin(yawRad) * cp));
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::mat4 Camera::viewMatrix() const {
    const glm::vec3 f = front();
    const glm::vec3 r = right();
    glm::vec3 up = glm::cross(r, f);
    if (std::abs(m_roll) > 1e-4f) {
        const float c = std::cos(glm::radians(m_roll));
        const float s = std::sin(glm::radians(m_roll));
        up = glm::normalize(up * c + r * s);
    }
    return glm::lookAt(m_position, m_position + f, up);
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, 0.1f, 400.0f);
}

} // namespace vox
