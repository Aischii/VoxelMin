#include "render/Shader.hpp"
#include "core/Log.hpp"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <vector>

namespace vox {

Shader::~Shader() {
    destroy();
}

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

uint32_t Shader::compile(uint32_t type, const std::string& source, const std::string& label) {
    uint32_t shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length > 1 ? static_cast<size_t>(length) : 1);
        glGetShaderInfoLog(shader, length, nullptr, message.data());
        log::error("Shader compile failed [%s]:\n%s", label.c_str(), message.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);
    if (vertexSource.empty() || fragmentSource.empty()) {
        log::error("Could not read shader files: %s / %s", vertexPath.c_str(), fragmentPath.c_str());
        return false;
    }

    uint32_t vertex = compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
    uint32_t fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return false;
    }

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length > 1 ? static_cast<size_t>(length) : 1);
        glGetProgramInfoLog(program, length, nullptr, message.data());
        log::error("Shader link failed: %s", message.data());
        glDeleteProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (m_program) glDeleteProgram(m_program);
    m_program = program;
    return true;
}

void Shader::use() const {
    if (m_program) glUseProgram(m_program);
}

void Shader::destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name), value);
}

void Shader::setVec2(const char* name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const char* name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace vox
