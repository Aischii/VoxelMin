# Sources & References

This document records all external open-source projects, mods, papers, and texture packs referenced or adapted in VoxelMin. Use this reference when reviewing architectural decisions or looking for future improvements.

---

## 1. FerriteCore (Memory & Data Structures)
- **Repository**: [https://github.com/malte0811/FerriteCore](https://github.com/malte0811/FerriteCore)
- **Author**: Malte0811
- **Domain**: Memory footprint optimization for Minecraft voxel engines.
- **Key Concepts & Implementations in VoxelMin**:
  - **Indexed Chunk Palettes**: Compact single-byte index storage with dynamically sized palettes (`ChunkPalette`), eliminating memory overhead for homogeneous chunks (e.g. empty air or deep stone).
  - **Run-Length Encoding (RLE)**: Compressing chunk voxel arrays into `RLERun` structures for disk persistence (`world.dat`) and network transmission.

---

## 2. ImmediatelyFast (Rendering Performance & Batching)
- **Repository**: [https://github.com/RaphiMC/ImmediatelyFast](https://github.com/RaphiMC/ImmediatelyFast)
- **Author**: RaphiMC
- **Domain**: Immediate-mode and batched buffer rendering optimizations.
- **Key Concepts & Implementations in VoxelMin**:
  - **Batch Drawing & State Minimization**: Grouping draw calls, minimizing OpenGL state swaps between UI elements and chunk geometry.
  - **Vertex Buffer Packing**: Compact vertex structures (position, UV, face tint/lighting packed into compact stride) for optimal GPU cache locality.
  - **HUD / GUI Immediate Batching**: Efficient 2D quad batching with procedural fonts and texture atlases.

---

## 3. Block Textures

### 3a. Style reference
- **Project**: VanillaHD-Mosaic by arogorn993-hue -- a free, open HD (256/512 px)
  resource pack for Minecraft.
- **Repository**: [https://github.com/arogorn993-hue/VanillaHD-Mosaic](https://github.com/arogorn993-hue/VanillaHD-Mosaic)
- **License**: Apache-2.0 (the pack's own LICENSE and NOTICE; the pack notes it
  includes AI-generated textures under the same terms).
- **Relationship to VoxelMin**: **style reference only -- no texture files were
  copied.** It is an HD pack and VoxelMin deliberately ships 16x16 pixel-art
  tiles in a 256x256 atlas, so the two are not resolution-compatible. The
  reference is for the clean, retro block-colour discipline, not the assets.

### 3b. Generation pipeline
- **Source**: the PNGs in `assets/textures/` are the project's own art, loaded at
  startup by `stbi_load` in `Texture::createAtlas()`.
- **Fallback**: every tile has a procedural generator, so a missing or undecodable
  PNG degrades that single block rather than breaking startup. 7 of the 17
  referenced PNGs are currently absent and render procedurally.
- **License**: the shipped PNGs are original project work. Anyone adding
  textures from a third-party pack must record that pack and its license here
  before shipping.

### 3c. Atlas & filtering
- Tiles are combined into a 256x256 GPU atlas (16x16 grid of 16x16 tiles;
  `config::ATLAS_PIXELS` / `TILE_PIXELS` in `src/core/Config.hpp`).
- Mipmapping uses `GL_NEAREST_MIPMAP_LINEAR` for minification with `GL_NEAREST`
  magnification, giving crisp retro pixels without distance shimmer.
- `Texture::createAtlas()` rescales any source PNG to 16x16 with
  `stbir_resize_uint8_linear`, so dropping in a 64x64 or 128x128 texture works
  without code changes.

---

## 4. AI-Improvements (Mob AI Task Filtering & Math Caching)
- **Repository**: [https://github.com/BuiltBrokenModding/AI-Improvements](https://github.com/BuiltBrokenModding/AI-Improvements)
- **Author**: BuiltBrokenModding
- **Domain**: A Minecraft mod that restructures entity AI so behaviours can be filtered per mob type, plus small maths caches.
- **Key Concepts & Implementations in VoxelMin**:
  - **Goal mask / filter layer** (`src/entity/MobAi.hpp`): every mob behaviour is a named bit in an `AiGoalMask` rather than an `if` buried in the update loop. Per-species goal sets live in one table (`kPigGoals`, `kCowGoals`), so disabling a behaviour for a species is a one-line edit instead of a code change. This is the same idea as the mod's filter layer, minus the config-file and reflection machinery a Java mod needs.
  - **Cached heading maths** (`Mob::updateForwardVector`): the mod caches its trig lookups; VoxelMin caches `sin`/`cos` of the mob's yaw and only recomputes when the heading actually changes, instead of calling `glm::radians` + `sin`/`cos` every mob every frame.
  - **Cheapest-test-first ordering**: goal bits are ordered so the goals that run most often (`LookAt`, `Float`, `StepUp`, `AvoidEdge`) are the ones that exit after a single block probe.
  - Note: the mod's *disable tasks for performance* angle is only partly applicable here. VoxelMin has ~24 mobs and no task scheduler, so the mask is used mainly to make behaviour configurable and to keep the AI readable, not to cut CPU cost.

---

## 5. Algorithms & Papers

These are the published algorithms the voxel code implements. They predate the
project and are reimplementations from the original descriptions.

- **Fast Voxel Traversal Algorithm (DDA)** -- John Amanatides & Andrew Woo,
  "A Fast Voxel Traversal Algorithm for Ray Tracing", Eurographics '87.
  [PDF](https://www.cs.yorku.ca/~amana/research/grid.pdf)
  Used in `src/world/Raycast.cpp` for block picking and in
  `EntityManager::raycast()` for entity hit tests.

- **Greedy Meshing** -- Mikola Lysenko, "Meshing in a Minecraft Game" (2012).
  <https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/>
  Used in `src/world/ChunkMesher.cpp` to merge coplanar quads per slice along
  all three axes, cutting the triangle count substantially versus per-face quads.

- **View Frustum Culling (Gribb-Hartmann plane extraction)** -- Gil Gribb &
  Klaus Hartmann, "Fast Extraction of Viewing Frustum Planes from the
  World-View-Projection Matrix".
  Implemented from the method in `src/render/Frustum.hpp`; the 6 planes are
  extracted from the combined view-projection matrix and tested against each
  chunk's AABB in `Renderer::drawWorld`.
  (The commonly cited course-hosted PDF has gone offline; the method is
  reproduced in most graphics texts, e.g. the frustum chapter of any real-time
  rendering reference.)

- **Voxel Palette Compression** -- inspired by [FerriteCore](https://github.com/maruohon/ferritecore),
  a Minecraft Forge library; see section 1 for how VoxelMin differs from it.

## 6. Libraries

- [GLFW](https://www.glfw.org/) -- windowing and input.
- [GLEW](https://github.com/nigels-com/glew) -- OpenGL extension loading.
- [GLM](https://github.com/g-truc/glm) -- maths types (`vec2`/`vec3`/`ivec3`,
  `mat4`).
- [stb](https://github.com/nothings/stb) -- single-header `stb_image` for PNG
  decoding and `stb_image_resize` for downscaling to 16x16. No other asset
  libraries are used.
