# Codebase Structuring & Modularization Plan

This document outlines the architectural refactoring plan for VoxelMin to improve modularity, maintainability, and clean separation of concerns as the project scales.

---

## 1. World & Item System Modularization

### Current State
- `src/world/Block.hpp` contains all `BlockId` enums, `TextureTile` mappings, `BlockDef` table, `ItemSlot` struct, tool durability calculations, harvestability logic, crafting tab catalogues, and block color lookups in a single large header.

### Planned Target Architecture
- **`src/world/block/BlockId.hpp`**:
  - Pure `enum class BlockId : uint8_t` and `enum class TextureTile : uint8_t`.
  - Block property query predicates (`isOpaque`, `isTransparent`, `isSolid`, `isLiquid`, `isLightSource`, `isPlant`).
- **`src/world/block/BlockDef.hpp`**:
  - `struct BlockDef` and `blockDef(BlockId)` table.
  - Voxel bounding box bounds (`blockBounds(BlockId)`).
- **`src/world/item/ItemSlot.hpp`**:
  - `struct ItemSlot` with count, durability, stack limits.
  - Item classifications (`isTool`, `isArmor`, `isFood`, `isConsumable`).
  - Tool durability tables and mining hardness formulas.
- **`src/world/item/Catalog.hpp`**:
  - Creative catalog groupings (Building, Tools & Combat, Food & Materials, Backrooms Anomalies).
  - Search index and catalog querying helpers.

---

## 2. World Generation Subsystem

### Current State
- Generators (`TerrainGenerator`, `VillageGenerator`, `BackroomsGenerator`) sit directly in `src/world/` alongside chunk storage, meshing, and raycasting.

### Planned Target Architecture
- **`src/world/gen/` Directory**:
  - **`TerrainGenerator.{hpp,cpp}`**: Multi-octave FastNoiseLite / OpenSimplex2 heightmaps, 3D noodle caves, ore veins, tree decorators, and surface biomes.
  - **`VillageGenerator.{hpp,cpp}`**: Procedural village layouts, surface contour alignment, road pathing, house/well/tower templates.
  - **`BackroomsGenerator.{hpp,cpp}`**: Level 0 infinite non-linear partition maze, blackout zone noise maps, acoustic drop ceiling fixtures, almond water caches, reality glitches, rare fire exits.
  - **`NoiseService.hpp`**: Centralized seeded noise provider wrapping FastNoiseLite with deterministic caching.

---

## 3. Dimension Management Architecture

### Current State
- Dimension switching logic and chunk cache swapping are mixed between `World` and `Application`.

### Planned Target Architecture
- **`src/world/dimension/Dimension.hpp`**:
  - `enum class DimensionId : uint8_t` and `struct DimensionInfo`.
- **`src/world/dimension/DimensionManager.{hpp,cpp}`**:
  - Manages per-dimension chunk storage caches (`m_overworldCache`, `m_backroomsCache`).
  - Safe transition calculation (`findSafeSpawn`, `findSafeOverworldReturn`) with transition cooldown timers.
  - Dimension-specific sky, fog, ambient lighting, and acoustic profile coordination.

---

## 4. Render Pipeline Modularization

### Current State
- `src/render/Renderer.cpp` handles 3D world drawing, sky rendering, entity batching, crack stages, 2D UI primitives, HUD survival vitals, Minecraft container GUI grids, Backrooms VHS glitch overlays, and F3 debug overlays.

### Planned Target Architecture
- **`src/render/WorldRenderer.{hpp,cpp}`**:
  - 3D chunk meshing passes (opaque + cutout passes with frustum culling).
  - Sky dome, sun/moon celestial rendering, stars, clouds.
  - Block break crack stages and block selection wireframes.
- **`src/render/UIRenderer.{hpp,cpp}`**:
  - 2D immediate-mode UI primitives (rectangles, gradients, textured icons).
  - HUD vitals (health hearts, hunger drumsticks, air bubbles, hotbar).
  - Container GUIs (Survival 2x2, Workbench 3x3, Creative tabs & search).
  - RPG title banners and F3 debug diagnostic overlays.
- **`src/render/PostProcess.{hpp,cpp}`**:
  - Underwater screen distortion and color grading.
  - Backrooms analog film grain, CRT scanlines, voltage flicker, and edge vignette.
  - Damage flash and hurt vignettes.

---

## 5. Implementation Roadmap & Milestones

1. **Phase 1 (Preparation)**: Extract `ItemSlot` and `Catalog` from `Block.hpp` without breaking API contracts.
2. **Phase 2 (Generators & Dimensions)**: Group generation algorithms into `src/world/gen/` and encapsulate dimension transitions in `DimensionManager`.
3. **Phase 3 (Renderer Decomposition)**: Decompose `Renderer.cpp` into focused `WorldRenderer`, `UIRenderer`, and `PostProcess` components.
4. **Phase 4 (Validation)**: Verify continuous zero-warning builds and automated regression checks across all stages.
