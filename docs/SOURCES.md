# Sources & References

This document records all external open-source projects, mods, papers, and texture packs referenced or adapted in VoxelMin. Use this reference when reviewing architectural decisions or looking for future improvements.

---

## 1. Sound Physics Remastered (Acoustic Simulation & DSP)
- **Repository**: [https://github.com/henkelmax/sound-physics-remastered](https://github.com/henkelmax/sound-physics-remastered)
- **Author**: Max Henkel (henkelmax)
- **Domain**: Physically based audio engine overhaul for voxel worlds providing acoustic ray tracing, realistic reverberation, occlusion, and absorption.
- **Key Concepts & Implementations for VoxelMin**:
  - **Acoustic Occlusion & High-Frequency Damping**: Voxel DDA raycasting between sound emitters (mobs, block breaks, footsteps) and the player's ears. Solid intervening blocks insert a biquad low-pass filter (muffling high frequencies) and lower dry volume so sounds around corners or behind walls sound realistic.
  - **Environment Reverberation & Room Evaluation**: Evaluating local cavern / room volume and enclosure density around the listener to dynamically compute reverb decay time, wet/dry reflection mix, and high-frequency damping factor. Large underground caverns produce lush echoes; open plains sound dry.
  - **Material-Specific Sound Absorption**: Differentiating absorption coefficients across block materials (e.g. wool and leaves absorb sound rapidly; smooth stone, cobblestone, and deepslate reflect high frequencies).
  - **Physically Based Attenuation**: Distance-based inverse-square audio falloff with stereo spatial panning.

---

## 2. Sodium & CaffeineMC (Modern Rendering Pipeline & Batching)
- **Repository**: [https://github.com/CaffeineMC/sodium](https://github.com/CaffeineMC/sodium) (formerly `sodium-fabric`)
- **Author**: CaffeineMC (jellysquid3 / IMS212)
- **Domain**: Next-generation chunk rendering engine and geometry pipeline for voxel engines.
- **Key Concepts & Implementations in VoxelMin**:
  - **Geometry Batching & State Consolidation**: Minimizing draw calls and GPU pipeline state transitions between opaque and transparent cutout passes.
  - **Compact Vertex Encodings**: Packing 3D world coordinates, texture UVs, vertex ambient occlusion, and dual-channel 4-bit lighting into a contiguous interleaved vertex stride for maximum GPU cache locality.
  - **Multi-Threaded Chunk Meshing**: Offloading procedural terrain noise, block palette compaction, and greedy meshing algorithms to background worker threads, freeing the main thread for smooth 60+ FPS rendering.
  - **Multi-Pass Culling**: Two-tier culling combining Gribb-Hartmann view frustum AABB extraction with face backface culling.

---

## 3. Presence Footsteps (Acoustic Material Cadence & Locomotion Dynamics)
- **Repository**: [https://github.com/Sollace/Presence-Footsteps](https://github.com/Sollace/Presence-Footsteps)
- **Author**: Sollace / Hurricaaane
- **Domain**: Dynamic acoustic locomotion and surface-responsive sound physics for voxel world characters.
- **Key Concepts & Implementations in VoxelMin**:
  - **Stride Cadence & Distance Accumulation**: Footstep audio triggers derived from true accumulated ground distance travelled rather than fixed timers, scaling dynamically with walking, sprinting, wading, and sneaking.
  - **Locomotion View Dynamics & Head Bobbing**: Harmonically linked camera translation, stride dips, lateral sway, and arm swing inertia coordinated with footfall impacts.
  - **Acoustic Material Differentiation**: Differentiating contact resonance across organic (grass/leaves), resonant (wood planks/logs), mineral (stone/cobblestone/ores), granular (dirt/sand/gravel), and fluid (water wading/submersion) substrate surfaces.

---

## 4. Baritone (Segmented Voxel Graph Pathfinding & AI)
- **Repository**: [https://github.com/cabaletta/baritone](https://github.com/cabaletta/baritone)
- **Author**: cabaletta / leijurv
- **Domain**: High-performance A* pathfinding and navigation across infinite voxel graphs.
- **Key Concepts & Implementations in VoxelMin**:
  - **Segmented 3D A* Search**: Navigating infinite, dynamically loaded chunk worlds by computing path segments toward destinations within the active loaded chunk radius.
  - **Voxel Graph Heuristics & Traversal Costs**: Evaluating jumping up 1 block, dropping down 1–3 blocks, swimming in water, or traversing paths with differentiated movement penalties.
  - **Collision Node Caching**: Evaluating block solidness and clearance rapidly without redundant memory allocations.

---

## 5. FerriteCore (Memory & Palette Optimization)
- **Repository**: [https://github.com/malte0811/FerriteCore](https://github.com/malte0811/FerriteCore)
- **Author**: Malte0811
- **Domain**: Memory footprint optimization for voxel engines.
- **Key Concepts & Implementations in VoxelMin**:
  - **Indexed Chunk Palettes**: Compact single-byte index storage with dynamically sized palettes (`ChunkPalette`), eliminating memory overhead for homogeneous chunks (e.g. empty air or deep stone).
  - **Run-Length Encoding (RLE)**: Compressing chunk voxel arrays into `RLERun` structures for disk persistence (`world.dat`) and stream payloads.

---

## 6. ImmediatelyFast (2D UI & Immediate-Mode Batching)
- **Repository**: [https://github.com/RaphiMC/ImmediatelyFast](https://github.com/RaphiMC/ImmediatelyFast)
- **Author**: RaphiMC
- **Domain**: Immediate-mode and batched buffer rendering optimizations.
- **Key Concepts & Implementations in VoxelMin**:
  - **HUD / GUI Immediate Batching**: Efficient 2D quad batching with procedural fonts and texture atlases.
  - **State Minimization**: Unified vertex buffers for HUD, inventory, and crosshair overlays.

## 7. AmbientSounds (Dynamic Biome Soundscapes & Nocturnal Acoustics)
- **Repository**: [https://github.com/CreativeMD/AmbientSounds](https://github.com/CreativeMD/AmbientSounds)
- **Author**: CreativeMD
- **Domain**: Context-sensitive procedural environmental soundscapes and dynamic biome audio cues.
- **Key Concepts & Implementations in VoxelMin**:
  - **Dynamic Biome Soundscapes**: Procedurally selecting acoustic background loops based on player position (Forest birds/rustling leaves, Plains wind, Desert whistling dunes, Ocean wave swells, Deep Cavern echo/water drips).
  - **Diurnal / Nocturnal Acoustic Transitions**: Blending daytime forest songbirds into nighttime crickets, owls, and swamp frogs as celestial sunlight changes.
  - **Altitude & Enclosure Audio Modifiers**: Triggering subterranean cave hums and high-altitude mountain winds based on surrounding voxel ceiling density and height.

---

## 8. Lithium (Physics, Collision & Entity Tick Optimization)
- **Repository**: [https://github.com/CaffeineMC/lithium](https://github.com/CaffeineMC/lithium)
- **Author**: CaffeineMC (2No2Name / IMS212 / Jellysquid)
- **Domain**: General-purpose physics, collision broadphase, and tick optimization for voxel engines.
- **Key Concepts & Implementations in VoxelMin**:
  - **Spatial Entity Partitioning**: Grid-based broadphase acceleration structures to reduce $O(N^2)$ entity hit tests and raycast queries.
  - **Voxel Collision Caching**: Fast chunk-coordinate hash lookup avoiding memory allocations during player and mob axis collision sweeps.
  - **Dormant Entity Sleep States**: Throttling AI updates and pathfinding for entities beyond active interaction radius.

---

## 9. MoreCulling (Block-Level & Entity Visibility Occlusion)
- **Repository**: [https://github.com/fxmorin/MoreCulling](https://github.com/fxmorin/MoreCulling)
- **Author**: fxmorin
- **Domain**: Advanced voxel face and entity occlusion culling.
- **Key Concepts & Implementations in VoxelMin**:
  - **Block-Level Occlusion Culling**: Culling hidden interior faces of non-solid/semi-transparent block boundaries (e.g. adjacent leaves, glass quads, fences).
  - **Entity Bounding-Box Occlusion**: Skipping render calls for mobs and item entities fully occluded behind solid terrain AABBs.
  - **Chunk Visibility Graph Traversal**: Skipping interior cave meshing and drawing when completely encapsulated by opaque chunk boundaries.

---

## 10. memoryLeakFix (Resource Lifecycle Management & Buffer Recycling)
- **Repository**: [https://github.com/fxmorin/memoryLeakFix](https://github.com/fxmorin/memoryLeakFix)
- **Author**: fxmorin
- **Domain**: Preventing resource leaks and managing deterministic buffer lifecycles in infinite voxel engines.
- **Key Concepts & Implementations in VoxelMin**:
  - **GPU Mesh Buffer Recycling Pool**: Recycling VAO/VBO/EBO handle allocations when streaming and unmeshing infinite chunks, avoiding GPU driver heap fragmentation.
  - **Chunk Eviction Cleanup**: Deterministic deallocation of unloaded chunk voxel palettes and light grids.
  - **Bounded Entity & Particle Lifecycles**: Strict capacity caps and time-to-live cleanup preventing particle system or dropped item memory leaks.

---

## 11. Falling Leaves (Environmental Canopy Particles & Flutter Physics)
- **Repository**: [https://github.com/Fourmisain/fallingleaves](https://github.com/Fourmisain/fallingleaves)
- **Author**: Fourmisain
- **Domain**: Procedural airborne foliage particles and atmospheric environmental visual effects.
- **Key Concepts & Implementations in VoxelMin**:
  - **Canopy Particle Spawners**: Tree leaf blocks exposed to air randomly spawn fluttering leaf particles.
  - **Aerodynamic Flutter Simulation**: Sinusoidal lateral wind sway, air drag, and gentle rotational oscillation during descent.
  - **Ground Surface Rest**: Particles land softly on solid ground or water surfaces before gradually fading.

---

## 12. Visuality (Dynamic Combat Sparks, Blood Splatter & Hit Feedback)
- **Repository**: [https://github.com/PinkGoosik/visuality](https://github.com/PinkGoosik/visuality)
- **Author**: PinkGoosik
- **Domain**: Dynamic combat particle feedback, impact sparks, critical strikes, and fluid splash dynamics.
- **Key Concepts & Implementations in VoxelMin**:
  - **Directional Melee Blood & Hit Splatter**: Dynamic crimson droplets ejected along the strike momentum vector, reacting to gravity and landing on surrounding blocks.
  - **Critical Strike Sparks & Slashing Arcs**: High-velocity gold-white impact spark particles and directional slash effects on high-damage tool strikes.
  - **Light-Reactive Particle Shading**: Combat and ambient particles dynamically shaded by local voxel sunlight and blocklight levels.

---

## 13. NetherPortalFix (Bidirectional Dimensional Coordinate Mapping)
- **Repository**: [https://github.com/TwelveIterations/NetherPortalFix](https://github.com/TwelveIterations/NetherPortalFix)
- **Author**: TwelveIterations (BlayTheNinth)
- **Domain**: Dimensional portal teleportation and exact 1:8 coordinate correspondence.
- **Key Concepts & Implementations in VoxelMin**:
  - **Bidirectional Portal Pairing**: Persisting exact linked entrance/exit portal coordinates to prevent stranded one-way dimension warping.
  - **Sub-Chunk Overworld $\leftrightarrow$ Nether Scaling**: Precise $8\times$ horizontal coordinate translation between Overworld $(X, Z)$ and Nether $(X/8, Z/8)$.

---

## 14. Block Textures

### 14a. Style reference
- **Project**: VanillaHD-Mosaic by arogorn993-hue -- a free, open HD (256/512 px) resource pack for Minecraft.
- **Repository**: [https://github.com/arogorn993-hue/VanillaHD-Mosaic](https://github.com/arogorn993-hue/VanillaHD-Mosaic)
- **License**: Apache-2.0.
- **Relationship to VoxelMin**: **Style reference only -- no texture files were copied.**

### 14b. Generation pipeline
- **Source**: The PNGs in `assets/textures/` are the project's own art, loaded at startup by `stbi_load` in `Texture::createAtlas()`.
- **Fallback**: Every tile has a procedural generator, so a missing or undecodable PNG degrades gracefully.

### 14c. Atlas & filtering
- Tiles are combined into a 256x256 GPU atlas (16x16 grid of 16x16 tiles; `config::ATLAS_PIXELS` / `TILE_PIXELS` in `src/core/Config.hpp`).
- Mipmapping uses `GL_NEAREST_MIPMAP_LINEAR` for minification with `GL_NEAREST` magnification.

---

## 15. AI-Improvements (Mob AI Task Filtering & Math Caching)
- **Repository**: [https://github.com/BuiltBrokenModding/AI-Improvements](https://github.com/BuiltBrokenModding/AI-Improvements)
- **Author**: BuiltBrokenModding
- **Domain**: Structuring entity AI into bitmask goal filters and trigonometric lookup caching.
- **Key Concepts & Implementations in VoxelMin**:
  - **Goal mask / filter layer** (`src/entity/MobAi.hpp`): Every mob behaviour is a named bit in an `AiGoalMask` (`kPigGoals`, `kCowGoals`, `kPigmanGoals`).
  - **Cached heading maths** (`Mob::updateForwardVector`): Caches `sin`/`cos` of heading yaw and only updates when orientation changes.

---

## 16. Algorithms & Papers

These are the published algorithms the voxel code implements:

- **Fast Voxel Traversal Algorithm (DDA)** -- John Amanatides & Andrew Woo, "A Fast Voxel Traversal Algorithm for Ray Tracing", Eurographics '87. [PDF](https://www.cs.yorku.ca/~amana/research/grid.pdf)
  Used in `src/world/Raycast.cpp` for block picking, entity hit tests, and sound occlusion rays.

- **Greedy Meshing** -- Mikola Lysenko, "Meshing in a Minecraft Game" (2012). <https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/>
  Used in `src/world/ChunkMesher.cpp` to merge coplanar quads along all three axes, cutting the triangle count substantially.

- **View Frustum Culling (Gribb-Hartmann plane extraction)** -- Gil Gribb & Klaus Hartmann, "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix".
  Used in `src/render/Frustum.hpp` to extract 6 frustum planes and cull chunk AABBs in `Renderer::drawWorld`.

---

## 17. FastNoiseLite (Coherent Procedural Noise & Terrain Generation)
- **Repository**: [https://github.com/Auburn/FastNoiseLite](https://github.com/Auburn/FastNoiseLite)
- **Author**: Jordan Peck (Auburn)
- **License**: MIT
- **Domain**: High-performance, zero-allocation coherent noise generation library in C++.
- **Key Concepts & Implementations in VoxelMin**:
  - **OpenSimplex2 & Multi-Octave Fractal FBm**: Continentalness, erosion, and biome macro-structures generating smooth continuous landmass transitions across infinite worlds.
  - **Ridged Multifractal Noise**: Carving dramatic alpine peaks, jagged ridges, and towering mountain summits reaching up to $Y = 76$.
  - **3D Simplex / Perlin Cavern Carving**: 3D interconnected noodle caves and cavern networks burrowing through bedrock to surface terrain.
  - **Zero-Allocation SIMD Performance**: Fast integer hashing and trigonometric caching enabling sub-millisecond chunk terrain noise evaluation.

---

## 18. The Backrooms Level 0 - "The Lobby / The Yellow Hell"
- **Reference**: [https://backrooms-wiki.wikidot.com/level-0](https://backrooms-wiki.wikidot.com/level-0)
- **Domain**: Procedural infinite non-linear liminal dimension, damp carpet mechanics, fluorescent humming light fixtures, and emergency exits.
- **Key Concepts & Implementations in VoxelMin**:
  - **Infinite Non-Linear Office Maze**: Cellular partitioned corridor networks with office support columns, dead-end rooms, mono-yellow wallpaper, moist damp carpet, and acoustic drop ceilings.
  - **Dimensional Noclip & Real-Time Transitions**: Stepping into or right-clicking `GlitchBlock` (reality tear) noclipped straight to Level 0; finding rare `ExitDoor` (emergency fire exit) returns to the Overworld.
  - **Atmospheric Sensory Haze & Title Banners**: Distinct mono-yellow volumetric fog, buzz-hum ceiling lights, dim fluorescent ambient shading, and cinematic RPG dimension title banner ("LEVEL 0 - The Yellow Hell").
  - **Survival Items**: `AlmondWater` consumable restore beverage restoring hunger and health.

---

## 19. Core Libraries

- [GLFW](https://www.glfw.org/) -- Windowing, OpenGL context, and input.
- [GLEW](https://github.com/nigels-com/glew) -- OpenGL 3.3 extension loading.
- [GLM](https://github.com/g-truc/glm) -- Maths library (`vec2`/`vec3`/`ivec3`, `mat4`, quaternions).
- [miniaudio](https://miniaud.io/) -- Audio playback, procedural synthesizers, and DSP filters.
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) -- Fast coherent noise generation library (`src/world/FastNoiseLite.hpp`).
- [stb](https://github.com/nothings/stb) -- Single-header `stb_image` for PNG decoding and `stb_image_resize` for downscaling.
