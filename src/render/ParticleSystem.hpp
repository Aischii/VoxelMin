#pragma once
#include "render/Camera.hpp"
#include "render/Shader.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

namespace vox {

class World;

struct Particle {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    glm::vec4 color{1.0f};
    float size = 0.08f;
    float life = 0.5f;
    float maxLife = 0.5f;
    float gravity = 0.0f;
};

class ParticleSystem {
public:
    ParticleSystem() = default;
    ~ParticleSystem();

    bool init();
    void shutdown();

    void spawnFlame(const glm::vec3& pos);
    void spawnSmoke(const glm::vec3& pos);
    void spawnDigParticles(const glm::vec3& blockPos, const glm::ivec3& normal, uint8_t blockId, int count = 4);
    void spawnBlockBreakParticles(const glm::vec3& blockPos, uint8_t blockId, int count = 24);

    void update(float dt, const World& world, const glm::vec3& playerPos);
    void render(const Camera& camera, float aspect);
    void clear();

    size_t particleCount() const { return m_particles.size(); }

private:
    struct ParticleVertex {
        glm::vec3 pos;
        glm::vec4 color;
    };

    std::vector<Particle> m_particles;
    Shader m_shader;
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    float m_spawnTimer = 0.0f;
};

} // namespace vox
