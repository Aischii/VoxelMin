#pragma once
#include <glm/glm.hpp>

namespace vox {

class World;

struct RayHit {
    bool hit = false;
    glm::ivec3 block{0}; // world coords of the hit block
    glm::ivec3 normal{0}; // face normal (axis of entry)
};

// Voxel ray march (Amanatides & Woo). Returns the first opaque block the ray
// enters within maxDistance, plus the face it entered through.
RayHit raycast(const World& world,
               const glm::vec3& origin,
               const glm::vec3& direction,
               float maxDistance);

} // namespace vox
