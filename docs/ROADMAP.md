# Roadmap

Ordered milestones. Each is a coherent, shippable increment. Update
`docs/PROGRESS.md` when a milestone completes.

---

# Version 1.0 Milestone Series

## v1.M1 -- Environmental Sound Physics & Spatial Acoustics (IN PROGRESS)
*Inspired by [Sound Physics Remastered](https://github.com/henkelmax/sound-physics-remastered), [Presence Footsteps](https://github.com/Sollace/Presence-Footsteps), and [AmbientSounds](https://github.com/CreativeMD/AmbientSounds)*

- **Ray-Traced Acoustic Occlusion**:
  - Perform voxel DDA raycasting between sound emitters (mobs, footsteps, block break/place, item drops) and the player's camera ears.
  - Intervening solid blocks insert dynamic low-pass biquad filtering (attenuating high frequencies) and adjust dry gain, realistically muffling sounds behind walls or around corners.
- **Dynamic Cave Reverberation & Echo Simulation**:
  - Sample spherical acoustic rays around the player position to estimate room volume, ceiling height, and surface enclosure ratio.
  - Large underground caverns, ravines, and high-ceiling stone rooms produce deep, lush reverberation tails and flutter echoes, while open plains remain crisp and dry.
- **Material-Specific Sound Absorption & Presence Locomotion**:
  - Voxel materials dynamically scale reflection absorption: Soft materials (Leaves, Wool, Grass, Dirt) absorb acoustic energy rapidly; Hard materials (Stone, Cobblestone, Bedrock, Deepslate) reflect clear high-frequency acoustic reflections.
  - Stride-accumulated footstep audio with substrate resonance differentiation across organic, mineral, resonant wood, and fluid blocks.
  - Authentic first-person view dynamics and weighted hand stride bobbing with user options toggle.
- **Underwater Acoustic Modeling**:
  - When player's head is submerged in water, apply underwater fluid low-pass muffling, pitch distortion, and bubbling ambient reverberation.
- **Dynamic Biome & Nocturnal Ambient Soundscapes**:
  - Contextual ambient audio loops blending daytime songs with nocturnal crickets and subterranean cave resonance.

---

## v1.M2 -- Multi-Biome Multi-Noise Generation & Dynamic Weather
*Inspired by [AmbientSounds](https://github.com/CreativeMD/AmbientSounds) and [TerraBlender](https://github.com/Glitchfiend/TerraBlender)*

- **Multi-Noise Biome Pipeline**:
  - Continuous 2D/3D noise maps (Continentalness, Erosion, Temperature, Humidity) generating varied biome transitions:
    - *Plains & Meadows*: Wild grass clusters, wildflowers, gentle rolling hills.
    - *Oak & Birch Forests*: Dense tree canopies, fallen logs, forest floor flora.
    - *Deserts*: Rolling sand dunes, cacti, dead bushes, sandstone strata.
    - *Snowy Taiga & Tundras*: Conical spruce trees, snow-covered surface layers, frosted leaves.
    - *High Mountain Peaks*: Jagged stone summits and exposed cliff faces.
    - *Oceans & Rivers*: Deep marine trenches, gravel beds, sandy beaches.
- **Dynamic Weather System & Ambient Acoustics**:
  - Dynamic weather cycles: Clear Skies, Overcast, Rainstorms, Thunderstorms, Snowstorms.
  - Particle weather sheets: Cascading rain streaks and drifting snow flakes attached to camera.
  - Sky darkening, lightning flashes with thunder audio delay, and ambient rainfall soundscapes (rain splashing on stone vs leaves vs water).
- **Underground Dungeons & Structures**:
  - Buried cobblestone/mossy dungeon chambers with loot chests, iron cages, and subterranean treasures.

---

## v1.M3 -- Survival Hostile Entities, Smelting & A* Pathfinding
*Inspired by [Baritone](https://github.com/cabaletta/baritone) and [Lithium](https://github.com/CaffeineMC/lithium)*

- **Hostile Mobs & Night Spawning**:
  - Zombies (melee brawler, group reinforcement) and Skeletons (ranged archers with bow draw animations and arrow physics).
  - Night and cave spawning restricted to dark areas (Blocklight $\le 7$ and Sunlight $\le 7$).
  - Sunlight burning: Undead mobs combust when exposed to direct morning sunlight.
- **Segmented 3D A* Voxel Grid Pathfinding**:
  - High-performance A* search on voxel graphs with node heuristics: 1-block step-ups, 1–3 block safe drops, swimming, avoiding hazards (fire/lava).
  - Multi-target tracking, line-of-sight checks, and obstacle avoidance.
- **Lithium-Inspired Spatial Physics & Tick Throttling**:
  - Spatial entity partitioning broadphase accelerating raycasts and collision tests.
  - Entity sleep and tick throttling for distant passive/hostile mobs beyond active radius.
- **Furnace & Smelting System**:
  - 3-slot Furnace workbench interface (Input Ore/Food, Fuel Source, Output Ingot/Cooked Food).
  - Dynamic flame burning timer and cook progress arrow.
  - Fuel values: Coal (8 items), Wood/Planks (1.5 items), Sticks (0.5 items).
- **Wooden Beds & Night-Skipping**:
  - Craftable wooden beds (3 Planks + 3 Wool).
  - Right-click to sleep at night: Fast-forwards time to morning (08:00 AM) and sets respawn point.

---

## v1.M4 -- Sub-Chunk Architecture, MoreCulling Occlusion & Worker Pool
*Inspired by [Sodium](https://github.com/CaffeineMC/sodium), [MoreCulling](https://github.com/fxmorin/MoreCulling), and [memoryLeakFix](https://github.com/fxmorin/memoryLeakFix)*

- **Vertical 16x16x16 Sub-Chunks (`ChunkSection`)**:
  - Subdividing vertical chunk columns into $16\times 16\times 16$ sections.
  - Empty air sections consume 0 GPU buffer memory and skip draw calls entirely.
  - Block modifications re-mesh only the affected $16\times 16\times 16$ section ($5\times$ faster updates), allowing world height expansion to $Y=256$.
- **MoreCulling Block-Level & Entity Occlusion**:
  - Block-face culling across semi-transparent blocks (leaves/glass/fences).
  - Entity AABB depth-frustum culling avoiding draws for mobs completely occluded behind terrain hills/caves.
- **Multi-Threaded Worker Pool**:
  - Background worker threads generating noise topography and computing greedy mesh quads off the main thread.
  - Main thread performs only sub-millisecond GPU vertex buffer uploads for stutter-free traversal at 16+ chunk render distance.
- **memoryLeakFix Buffer Recycling Pool**:
  - GPU VAO/VBO/EBO handle recycling ring buffer for streamed chunk allocations, preventing OpenGL driver memory leaks during infinite world exploration.

---

## v1.M5 -- The Nether Dimension & Portals

- **Obsidian & Portal Ignition**:
  - Obsidian blocks created when water contacts lava sources.
  - $4\times 5$ Obsidian portal frame activated by Flint & Steel with animated purple portal vortex.
- **The Nether World Generation**:
  - Enclosed cavernous dimension with Bedrock roof ($Y=128$) and floor ($Y=0$).
  - Netherrack terrain, sprawling Lava Oceans, Glowstone stalactites, Soul Sand valleys.
  - Separate chunk dimension storage and instantaneous teleportation transition.

---

# Version 0 Legacy Archive (Delivered)

### M9 -- Smooth Dynamic Lighting, Infinite World Generation & Modern UI -- DONE
- **Infinite World Generation**: $64$-bit SplitMix chunk map, Euclidean floor-division coordinates, continuous chunk streaming around player.
- **Fast Spawn Generation**: $5\times 5$ initial spawn grid loaded in $<150\text{ ms}$.
- **4-Corner Per-Vertex Smooth Lighting**: Vertex smooth lighting sampling neighbor voxels along face tangents/bitangents with non-linear perceptual gamma curve.
- **Modern Minecraft UI & Crafting**: Survival Inventory (Armor + Avatar + Offhand + 2x2 Crafting), Creative Catalog with search & tabs, and 3x3 Crafting Table.
- **First-Person Viewmodel**: Extended 3D arm with 2.5D tool sprites and 3D mini-blocks.

### M8 -- Console Crafting, RPG HUD & Ecological Survival -- DONE
- Legacy Console Crafting UI, 3x3 Crafting Table, RPG MMO Vitals Card HUD, 8-phase lunar cycle, world ecology (dirt spread / grass collapse), 'Q'/Ctrl+'Q' item toss physics.

### M7 -- Dual-Channel BFS Lighting & Dynamic Soundtracks -- DONE
- 4-bit Sunlight & Blocklight BFS propagation, pitch black darkness, procedural arpeggio title music & exploration soundscapes with dynamic crossfading.

### M6 -- Miniaudio Engine & Persistence Protection -- DONE
- Procedural stereo audio engine, spatialized sound effects, persistent multi-path save protection.

### M5 -- Gameplay Entities & Scale Expansion -- DONE
- Autonomous Pigs, Cows, Pigman Villagers, multi-template surface villages, 3D wall torches, particle systems, F5 camera perspectives.

### M1–M4 -- Foundation, Menus, Polish, Caves & Ores -- DONE
- Voxel DDA raycasting, greedy meshing, Gribb-Hartmann frustum culling, RLE saves, caves, mineral ores.

---

## See also

- [`PROGRESS.md`](PROGRESS.md) -- Living status, version history, metrics.
- [`SOURCES.md`](SOURCES.md) -- External references, algorithms, and mod architecture citations.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) -- Codebase map and design patterns.
