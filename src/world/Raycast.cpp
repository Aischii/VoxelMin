#include "world/Raycast.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace vox {
namespace {

bool rayAABB(const glm::vec3& origin, const glm::vec3& dir,
             const glm::vec3& bMin, const glm::vec3& bMax,
             float& outDist, glm::ivec3& outNormal) {
    float tMin = 0.0f;
    float tMax = 1e9f;
    glm::ivec3 nearNormal(0);

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-6f) {
            if (origin[i] < bMin[i] || origin[i] > bMax[i]) return false;
        } else {
            float t1 = (bMin[i] - origin[i]) / dir[i];
            float t2 = (bMax[i] - origin[i]) / dir[i];
            glm::ivec3 n1(0), n2(0);
            n1[i] = (dir[i] > 0.0f) ? -1 : 1;
            n2[i] = -n1[i];
            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }
            if (t1 > tMin) {
                tMin = t1;
                nearNormal = n1;
            }
            if (t2 < tMax) {
                tMax = t2;
            }
            if (tMin > tMax) return false;
        }
    }
    outDist = tMin;
    outNormal = nearNormal;
    return true;
}

} // namespace

RayHit raycast(const World& world,
               const glm::vec3& origin,
               const glm::vec3& direction,
               float maxDistance) {
    RayHit result;

    const glm::vec3 dir = glm::normalize(direction);
    int x = static_cast<int>(std::floor(origin.x));
    int y = static_cast<int>(std::floor(origin.y));
    int z = static_cast<int>(std::floor(origin.z));

    const int stepX = (dir.x > 0.0f) ? 1 : ((dir.x < 0.0f) ? -1 : 0);
    const int stepY = (dir.y > 0.0f) ? 1 : ((dir.y < 0.0f) ? -1 : 0);
    const int stepZ = (dir.z > 0.0f) ? 1 : ((dir.z < 0.0f) ? -1 : 0);

    const float inf = std::numeric_limits<float>::infinity();

    const float tDeltaX = (stepX != 0) ? 1.0f / std::abs(dir.x) : inf;
    const float tDeltaY = (stepY != 0) ? 1.0f / std::abs(dir.y) : inf;
    const float tDeltaZ = (stepZ != 0) ? 1.0f / std::abs(dir.z) : inf;

    float tMaxX = (stepX != 0)
        ? ((stepX > 0 ? (x + 1 - origin.x) : (origin.x - x)) / std::abs(dir.x)) : inf;
    float tMaxY = (stepY != 0)
        ? ((stepY > 0 ? (y + 1 - origin.y) : (origin.y - y)) / std::abs(dir.y)) : inf;
    float tMaxZ = (stepZ != 0)
        ? ((stepZ > 0 ? (z + 1 - origin.z) : (origin.z - z)) / std::abs(dir.z)) : inf;

    glm::ivec3 normal(0);
    float travelled = 0.0f;

    while (travelled <= maxDistance) {
        const BlockId b = world.getBlock(x, y, z);
        if (!isAir(b) && !isLiquid(b)) {
            const BlockBounds bounds = blockBounds(b);
            const bool isFullCube = (bounds.minOffset == glm::vec3(0.0f) && bounds.maxOffset == glm::vec3(1.0f));
            if (isFullCube) {
                result.hit = true;
                result.block = {x, y, z};
                result.normal = normal;
                return result;
            } else {
                const glm::vec3 bMin = glm::vec3(x, y, z) + bounds.minOffset;
                const glm::vec3 bMax = glm::vec3(x, y, z) + bounds.maxOffset;
                float dist = 0.0f;
                glm::ivec3 subNormal(0);
                if (rayAABB(origin, dir, bMin, bMax, dist, subNormal) && dist <= maxDistance) {
                    result.hit = true;
                    result.block = {x, y, z};
                    result.normal = subNormal;
                    return result;
                }
            }
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX; travelled = tMaxX; tMaxX += tDeltaX; normal = {-stepX, 0, 0};
            } else {
                z += stepZ; travelled = tMaxZ; tMaxZ += tDeltaZ; normal = {0, 0, -stepZ};
            }
        } else {
            if (tMaxY < tMaxZ) {
                y += stepY; travelled = tMaxY; tMaxY += tDeltaY; normal = {0, -stepY, 0};
            } else {
                z += stepZ; travelled = tMaxZ; tMaxZ += tDeltaZ; normal = {0, 0, -stepZ};
            }
        }
    }

    return result;
}

} // namespace vox
