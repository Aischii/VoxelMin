#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace vox {

// Vertex layout used by chunk geometry:
//   location 0 -> position (vec3)
//   location 1 -> normal   (vec3)
//   location 2 -> uv       (vec2)
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec2 tileMin;
    glm::vec2 tileSize;
    float ao;
    float light;
};

// Thin OpenGL VAO/VBO/EBO wrapper. Move-only via explicit destroy().
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void upload(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void draw() const;
    void destroy();

    bool empty() const { return m_indexCount == 0; }
    // Index count of the last upload; the F3 overlay uses it for triangle counts.
    uint32_t indexCount() const { return m_indexCount; }
    uint32_t triangleCount() const { return m_indexCount / 3; }

private:
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    uint32_t m_indexCount = 0;
};

} // namespace vox
