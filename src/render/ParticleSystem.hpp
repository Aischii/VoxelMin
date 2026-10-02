#pragma once
#include "render/Camera.hpp"
#include "render/Shader.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace vox {

class World;

enum class ParticleType : uint8_t {
    Generic = 0,
    Flame,
    Smoke,
    Leaf,
    Blood,
    Spark
};

struct Particle {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    glm::vec4 color{1.0f};
    float size = 0.08f;
    float life = 0.5f;
    float maxLife = 0.5f;
    float gravity = 0.0f;
    ParticleType type = ParticleType::Generic;
    float swayPhase = 0.0f;
    float swaySpeed = 2.5f;
    bool resting = false;
};

class ParticleSystem {
public:
    ParticleSystem() = default;
    ~ParticleSystem();

    bool init();
    void shutdown();

    void spawnFlame(const glm::vec3& pos);
    void spawnFire(const glm::vec3& pos, int count = 1);
    void spawnSmoke(const glm::vec3& pos);
    void spawnFallingLeaf(const glm::vec3& pos, const glm::vec3& color = glm::vec3(0.24f, 0.65f, 0.18f));
    void spawnHitParticles(const glm::vec3& pos, const glm::vec3& hitDir, bool isCrit = false, int count = 12);
    void spawnBloodSplatter(const glm::vec3& pos, const glm::vec3& hitDir, int count = 16);
    void spawnDigParticles(const glm::vec3& blockPos, const glm::ivec3& normal, uint8_t blockId,
                           const World& world, float sunlight = 1.0f, int count = 4);
    void spawnBlockBreakParticles(const glm::vec3& blockPos, uint8_t blockId,
                                  const World& world, float sunlight = 1.0f, int count = 24);

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
    float m_leafScanTimer = 0.0f;
};

} // namespace vox
