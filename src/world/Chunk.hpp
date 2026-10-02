#pragma once
#include "core/Config.hpp"
#include "render/Mesh.hpp"
#include "world/Block.hpp"

#include <cstdint>
#include <vector>

namespace vox {

// ---------------------------------------------------------------------------
// Single RLE run for world save compression (FerriteCore / RLE pattern).
// ---------------------------------------------------------------------------
struct RLERun {
    uint16_t count;
    BlockId block;
};

// ---------------------------------------------------------------------------
// ChunkPalette implements palette-compressed voxel storage (FerriteCore style).
// - Uniform single-block chunks (e.g. 100% Air or Stone) consume 0 index array
//   bytes (1 byte palette).
// - Multi-block chunks use an indexed 8-bit palette array + distinct block table.
// ---------------------------------------------------------------------------
class ChunkPalette {
public:
    static constexpr size_t TotalVoxels = static_cast<size_t>(config::CHUNK_SIZE_X) *
                                          config::CHUNK_SIZE_Y * config::CHUNK_SIZE_Z;

    ChunkPalette() : m_isSingle(true), m_singleBlock(BlockId::Air) {}

    BlockId get(size_t idx) const {
        if (m_isSingle) return m_singleBlock;
        return m_palette[m_indices[idx]];
    }

    void set(size_t idx, BlockId block) {
        if (m_isSingle) {
            if (m_singleBlock == block) return;
            m_isSingle = false;
            m_palette = { m_singleBlock };
            m_indices.assign(TotalVoxels, 0);
        }

        const uint8_t palIdx = getOrAddPaletteIndex(block);
        m_indices[idx] = palIdx;
    }

    void fill(BlockId block) {
        m_isSingle = true;
        m_singleBlock = block;
        m_palette.clear();
        m_indices.clear();
        m_indices.shrink_to_fit();
    }

    size_t memoryUsage() const {
        if (m_isSingle) return sizeof(ChunkPalette);
        return sizeof(ChunkPalette) + m_palette.size() * sizeof(BlockId) + m_indices.size() * sizeof(uint8_t);
    }

    bool isSingle() const { return m_isSingle; }
    BlockId singleBlock() const { return m_singleBlock; }
    const std::vector<BlockId>& palette() const { return m_palette; }
    const std::vector<uint8_t>& indices() const { return m_indices; }

    void compact() {
        if (m_isSingle || m_indices.empty()) return;
        const uint8_t first = m_indices[0];
        for (size_t i = 1; i < m_indices.size(); ++i) {
            if (m_indices[i] != first) return;
        }
        fill(m_palette[first]);
    }

private:
    uint8_t getOrAddPaletteIndex(BlockId block) {
        for (size_t i = 0; i < m_palette.size(); ++i) {
            if (m_palette[i] == block) return static_cast<uint8_t>(i);
        }
        m_palette.push_back(block);
        return static_cast<uint8_t>(m_palette.size() - 1);
    }

    bool m_isSingle = true;
    BlockId m_singleBlock = BlockId::Air;
    std::vector<BlockId> m_palette;
    std::vector<uint8_t> m_indices;
};

// ---------------------------------------------------------------------------
// A Chunk is a fixed W x H x D block of voxels plus the GPU mesh built from it.
// ---------------------------------------------------------------------------
class Chunk {
public:
    static constexpr int W = config::CHUNK_SIZE_X;
    static constexpr int H = config::CHUNK_SIZE_Y;
    static constexpr int D = config::CHUNK_SIZE_Z;

    Chunk(int cx, int cz)
        : m_cx(cx), m_cz(cz), m_light(static_cast<size_t>(W) * H * D, 0) {
        m_blocks.fill(BlockId::Air);
    }

    int chunkX() const { return m_cx; }
    int chunkZ() const { return m_cz; }
    int originX() const { return m_cx * W; }
    int originZ() const { return m_cz * D; }

    BlockId get(int x, int y, int z) const {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return BlockId::Air;
        return m_blocks.get(static_cast<size_t>(index(x, y, z)));
    }

    void set(int x, int y, int z, BlockId b) {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return;
        m_blocks.set(static_cast<size_t>(index(x, y, z)), b);
    }

    uint8_t getSunLight(int x, int y, int z) const {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return (y >= H) ? 15 : 0;
        return (m_light[static_cast<size_t>(index(x, y, z))] >> 4) & 0x0F;
    }

    uint8_t getBlockLight(int x, int y, int z) const {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return 0;
        return m_light[static_cast<size_t>(index(x, y, z))] & 0x0F;
    }

    void setSunLight(int x, int y, int z, uint8_t level) {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return;
        const size_t idx = static_cast<size_t>(index(x, y, z));
        m_light[idx] = static_cast<uint8_t>((m_light[idx] & 0x0F) | ((level & 0x0F) << 4));
    }

    void setBlockLight(int x, int y, int z, uint8_t level) {
        if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return;
        const size_t idx = static_cast<size_t>(index(x, y, z));
        m_light[idx] = static_cast<uint8_t>((m_light[idx] & 0xF0) | (level & 0x0F));
    }

    void clearLight() {
        std::fill(m_light.begin(), m_light.end(), 0);
    }

    static constexpr int index(int x, int y, int z) { return (y * D + z) * W + x; }

    size_t memoryUsage() const { return m_blocks.memoryUsage() + m_light.size() * sizeof(uint8_t); }
    void compactStorage() { m_blocks.compact(); }

    // RLE compression for disk saves & stream payloads.
    std::vector<RLERun> rleCompress() const {
        std::vector<RLERun> runs;
        const size_t count = static_cast<size_t>(W) * H * D;
        if (count == 0) return runs;

        BlockId current = m_blocks.get(0);
        uint16_t currentCount = 1;

        for (size_t i = 1; i < count; ++i) {
            BlockId b = m_blocks.get(i);
            if (b == current && currentCount < 65535) {
                currentCount++;
            } else {
                runs.push_back({ currentCount, current });
                current = b;
                currentCount = 1;
            }
        }
        runs.push_back({ currentCount, current });
        return runs;
    }

    void decompressRle(const std::vector<RLERun>& runs) {
        size_t idx = 0;
        const size_t count = static_cast<size_t>(W) * H * D;
        for (const auto& run : runs) {
            for (uint16_t i = 0; i < run.count && idx < count; ++i) {
                m_blocks.set(idx++, run.block);
            }
        }
        m_blocks.compact();
        dirty = true;
    }

    Mesh mesh;                 // Opaque GPU geometry
    Mesh transparentMesh;      // Transparent/cutout GPU geometry
    bool dirty = true;         // needs (re)meshing
    bool terrainGenerated = false; // base terrain noise/blocks generated

private:
    int m_cx, m_cz;
    ChunkPalette m_blocks;
    std::vector<uint8_t> m_light;
};

} // namespace vox
