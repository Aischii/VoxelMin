#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cmath>

namespace vox {

// ---------------------------------------------------------------------------
// View Frustum extracted from the view-projection matrix.
// Used for high-performance Chunk AABB Frustum Culling.
// ---------------------------------------------------------------------------
class Frustum {
public:
    struct Plane {
        glm::vec3 normal{0.0f};
        float distance = 0.0f;

        void normalize() {
            const float len = glm::length(normal);
            if (len > 1e-6f) {
                normal /= len;
                distance /= len;
            }
        }

        float distanceToPoint(const glm::vec3& p) const {
            return glm::dot(normal, p) + distance;
        }
    };

    void update(const glm::mat4& vp) {
        // Gribb-Hartmann plane extraction
        // Left
        m_planes[0].normal.x = vp[0][3] + vp[0][0];
        m_planes[0].normal.y = vp[1][3] + vp[1][0];
        m_planes[0].normal.z = vp[2][3] + vp[2][0];
        m_planes[0].distance = vp[3][3] + vp[3][0];

        // Right
        m_planes[1].normal.x = vp[0][3] - vp[0][0];
        m_planes[1].normal.y = vp[1][3] - vp[1][0];
        m_planes[1].normal.z = vp[2][3] - vp[2][0];
        m_planes[1].distance = vp[3][3] - vp[3][0];

        // Bottom
        m_planes[2].normal.x = vp[0][3] + vp[0][1];
        m_planes[2].normal.y = vp[1][3] + vp[1][1];
        m_planes[2].normal.z = vp[2][3] + vp[2][1];
        m_planes[2].distance = vp[3][3] + vp[3][1];

        // Top
        m_planes[3].normal.x = vp[0][3] - vp[0][1];
        m_planes[3].normal.y = vp[1][3] - vp[1][1];
        m_planes[3].normal.z = vp[2][3] - vp[2][1];
        m_planes[3].distance = vp[3][3] - vp[3][1];

        // Near
        m_planes[4].normal.x = vp[0][3] + vp[0][2];
        m_planes[4].normal.y = vp[1][3] + vp[1][2];
        m_planes[4].normal.z = vp[2][3] + vp[2][2];
        m_planes[4].distance = vp[3][3] + vp[3][2];

        // Far
        m_planes[5].normal.x = vp[0][3] - vp[0][2];
        m_planes[5].normal.y = vp[1][3] - vp[1][2];
        m_planes[5].normal.z = vp[2][3] - vp[2][2];
        m_planes[5].distance = vp[3][3] - vp[3][2];

        for (auto& plane : m_planes) {
            plane.normalize();
        }
    }

    bool isBoxVisible(const glm::vec3& minP, const glm::vec3& maxP) const {
        for (const auto& plane : m_planes) {
            // Find p-vertex (the positive vertex along the normal)
            glm::vec3 p = minP;
            if (plane.normal.x >= 0.0f) p.x = maxP.x;
            if (plane.normal.y >= 0.0f) p.y = maxP.y;
            if (plane.normal.z >= 0.0f) p.z = maxP.z;

            if (plane.distanceToPoint(p) < 0.0f) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<Plane, 6> m_planes;
};

} // namespace vox
