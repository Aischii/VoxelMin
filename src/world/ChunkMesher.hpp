#pragma once
#include "render/Mesh.hpp"

#include <cstdint>
#include <vector>

namespace vox {

class World;
class Chunk;

// Builds greedy-merged vertex/index buffers for a chunk into opaque and
// transparent streams. Faces shared with opaque neighbours are culled.
void buildChunkGeometry(const World& world,
                        const Chunk& chunk,
                        std::vector<Vertex>& outOpaqueVertices,
                        std::vector<uint32_t>& outOpaqueIndices,
                        std::vector<Vertex>& outTransVertices,
                        std::vector<uint32_t>& outTransIndices);

} // namespace vox
