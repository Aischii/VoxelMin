#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace vox {

// Minimal GLSL program wrapper with a handful of uniform setters.
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const;
    void destroy();

    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, const glm::vec2& value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setVec4(const char* name, const glm::vec4& value) const;
    void setMat4(const char* name, const glm::mat4& value) const;

    uint32_t id() const { return m_program; }

private:
    static uint32_t compile(uint32_t type, const std::string& source, const std::string& label);
    static std::string readFile(const std::string& path);

    uint32_t m_program = 0;
};

} // namespace vox
