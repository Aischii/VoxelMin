#pragma once
#include <cstdint>

namespace vox {

// Procedurally generated block texture atlas. No image files needed: every
// tile is drawn in code at startup (see Texture.cpp).
class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void createAtlas();
    void bind(uint32_t unit = 0) const;
    void destroy();

    uint32_t id() const { return m_id; }

private:
    uint32_t m_id = 0;
};

} // namespace vox
