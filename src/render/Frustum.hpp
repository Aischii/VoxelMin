#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cmath>

namespace vox {

// ---------------------------------------------------------------------------
// View Frustum: 6-Plane Extraction and Fast Geometric AABB / Sphere Culling.
// References:
// - LearnOpenGL Frustum Culling: https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
// - Gribb-Hartmann Plane Extraction: "Fast Extraction of Viewing Frustum Planes"
// - Wikipedia Viewing Frustum: https://en.wikipedia.org/wiki/Frustum
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

    // Extracts normalized frustum clipping planes from view-projection matrix
    void update(const glm::mat4& vp) {
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

    // Branchless Center-Extents AABB test (LearnOpenGL / Ericson Real-Time Collision)
    bool isBoxVisibleCenterExtents(const glm::vec3& center, const glm::vec3& extents) const {
        for (const auto& plane : m_planes) {
            const float r = extents.x * std::abs(plane.normal.x) +
                            extents.y * std::abs(plane.normal.y) +
                            extents.z * std::abs(plane.normal.z);
            if (plane.distanceToPoint(center) < -r) {
                return false;
            }
        }
        return true;
    }

    // Overload taking minimum and maximum bounding box corners
    bool isBoxVisible(const glm::vec3& minP, const glm::vec3& maxP) const {
        const glm::vec3 center = (minP + maxP) * 0.5f;
        const glm::vec3 extents = (maxP - minP) * 0.5f;
        return isBoxVisibleCenterExtents(center, extents);
    }

    // Bounding Sphere Frustum Test (Ideal for rapid mob & particle emitter culling)
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : m_planes) {
            if (plane.distanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    // Point Frustum Test
    bool isPointVisible(const glm::vec3& point) const {
        for (const auto& plane : m_planes) {
            if (plane.distanceToPoint(point) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    const std::array<Plane, 6>& planes() const { return m_planes; }

private:
    std::array<Plane, 6> m_planes;
};

} // namespace vox
