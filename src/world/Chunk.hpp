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
// ChunkPalette implements dynamic bit-packed palette compression
// (Voxel Wiki / FerriteCore architecture).
// - Uniform single-block chunks (e.g. 100% Air or Stone): 0 bits/voxel (1 byte).
// - 2 unique blocks: 1 bit/voxel (2,560 bytes).
// - 3–4 unique blocks: 2 bits/voxel (5,120 bytes).
// - 5–16 unique blocks: 4 bits/voxel (10,240 bytes).
// - 17+ unique blocks: 8 bits/voxel (20,480 bytes).
// ---------------------------------------------------------------------------
class ChunkPalette {
public:
    static constexpr size_t TotalVoxels = static_cast<size_t>(config::CHUNK_SIZE_X) *
                                          config::CHUNK_SIZE_Y * config::CHUNK_SIZE_Z;

    ChunkPalette() : m_bitsPerVoxel(0), m_palette({ BlockId::Air }) {}

    BlockId get(size_t idx) const {
        if (m_palette.empty()) return BlockId::Air;
        if (m_bitsPerVoxel == 0) return m_palette[0];
        const uint8_t raw = getRawIndex(idx);
        if (raw < m_palette.size()) return m_palette[raw];
        return BlockId::Air;
    }

    void set(size_t idx, BlockId block) {
        if (m_palette.empty()) {
            fill(block);
            return;
        }

        if (m_bitsPerVoxel == 0) {
            if (m_palette[0] == block) return;
            m_palette.push_back(block);
            promote(1);
            setRawIndex(idx, 1);
            return;
        }

        // Check if block already exists in palette
        for (size_t i = 0; i < m_palette.size(); ++i) {
            if (m_palette[i] == block) {
                setRawIndex(idx, static_cast<uint8_t>(i));
                return;
            }
        }

        // New unique block
        const uint8_t newPalIdx = static_cast<uint8_t>(m_palette.size());
        m_palette.push_back(block);

        if (m_palette.size() > 16 && m_bitsPerVoxel < 8) {
            promote(8);
        } else if (m_palette.size() > 4 && m_bitsPerVoxel < 4) {
            promote(4);
        } else if (m_palette.size() > 2 && m_bitsPerVoxel < 2) {
            promote(2);
        }

        setRawIndex(idx, newPalIdx);
    }

    void fill(BlockId block) {
        m_bitsPerVoxel = 0;
        m_palette = { block };
        m_packedData.clear();
        m_packedData.shrink_to_fit();
    }

    size_t memoryUsage() const {
        return sizeof(ChunkPalette) +
               m_palette.size() * sizeof(BlockId) +
               m_packedData.size() * sizeof(uint8_t);
    }

    bool isSingle() const { return m_bitsPerVoxel == 0; }
    BlockId singleBlock() const { return m_palette.empty() ? BlockId::Air : m_palette[0]; }
    const std::vector<BlockId>& palette() const { return m_palette; }
    uint8_t bitsPerVoxel() const { return m_bitsPerVoxel; }

    void compact() {
        if (m_palette.empty() || m_bitsPerVoxel == 0) return;

        std::vector<bool> used(m_palette.size(), false);
        size_t uniqueUsed = 0;
        uint8_t singleIdx = 0;

        for (size_t i = 0; i < TotalVoxels; ++i) {
            uint8_t p = getRawIndex(i);
            if (p < m_palette.size() && !used[p]) {
                used[p] = true;
                uniqueUsed++;
                singleIdx = p;
            }
        }

        if (uniqueUsed <= 1) {
            BlockId b = (m_palette.empty()) ? BlockId::Air : m_palette[singleIdx];
            fill(b);
            return;
        }

        uint8_t targetBits = 8;
        if (uniqueUsed <= 2) targetBits = 1;
        else if (uniqueUsed <= 4) targetBits = 2;
        else if (uniqueUsed <= 16) targetBits = 4;

        std::vector<BlockId> newPalette;
        std::vector<uint8_t> oldToNew(m_palette.size(), 0);
        newPalette.reserve(uniqueUsed);

        for (size_t i = 0; i < m_palette.size(); ++i) {
            if (used[i]) {
                oldToNew[i] = static_cast<uint8_t>(newPalette.size());
                newPalette.push_back(m_palette[i]);
            }
        }

        std::vector<uint8_t> oldIndices;
        oldIndices.reserve(TotalVoxels);
        for (size_t i = 0; i < TotalVoxels; ++i) {
            uint8_t oldP = getRawIndex(i);
            oldIndices.push_back((oldP < oldToNew.size()) ? oldToNew[oldP] : 0);
        }

        m_palette = std::move(newPalette);
        m_bitsPerVoxel = targetBits;

        size_t newBytes = 0;
        if (m_bitsPerVoxel == 1) newBytes = (TotalVoxels + 7) / 8;
        else if (m_bitsPerVoxel == 2) newBytes = (TotalVoxels + 3) / 4;
        else if (m_bitsPerVoxel == 4) newBytes = (TotalVoxels + 1) / 2;
        else if (m_bitsPerVoxel == 8) newBytes = TotalVoxels;

        m_packedData.assign(newBytes, 0);
        for (size_t i = 0; i < TotalVoxels; ++i) {
            setRawIndex(i, oldIndices[i]);
        }
    }

private:
    uint8_t getRawIndex(size_t idx) const {
        switch (m_bitsPerVoxel) {
            case 0: return 0;
            case 1: return (m_packedData[idx >> 3] >> (idx & 7)) & 1;
            case 2: return (m_packedData[idx >> 2] >> ((idx & 3) << 1)) & 3;
            case 4: return (m_packedData[idx >> 1] >> ((idx & 1) << 2)) & 0x0F;
            case 8: return m_packedData[idx];
            default: return 0;
        }
    }

    void setRawIndex(size_t idx, uint8_t palIdx) {
        switch (m_bitsPerVoxel) {
            case 1: {
                const uint8_t shift = static_cast<uint8_t>(idx & 7);
                uint8_t& b = m_packedData[idx >> 3];
                b = static_cast<uint8_t>((b & ~(1u << shift)) | ((palIdx & 1u) << shift));
                break;
            }
            case 2: {
                const uint8_t shift = static_cast<uint8_t>((idx & 3) << 1);
                uint8_t& b = m_packedData[idx >> 2];
                b = static_cast<uint8_t>((b & ~(3u << shift)) | ((palIdx & 3u) << shift));
                break;
            }
            case 4: {
                const uint8_t shift = static_cast<uint8_t>((idx & 1) << 2);
                uint8_t& b = m_packedData[idx >> 1];
                b = static_cast<uint8_t>((b & ~(0x0Fu << shift)) | ((palIdx & 0x0Fu) << shift));
                break;
            }
            case 8: {
                m_packedData[idx] = palIdx;
                break;
            }
            default: break;
        }
    }

    void promote(uint8_t newBits) {
        if (newBits <= m_bitsPerVoxel) return;

        std::vector<uint8_t> oldIndices;
        oldIndices.reserve(TotalVoxels);
        for (size_t i = 0; i < TotalVoxels; ++i) {
            oldIndices.push_back(getRawIndex(i));
        }

        m_bitsPerVoxel = newBits;
        size_t newBytes = 0;
        if (m_bitsPerVoxel == 1) newBytes = (TotalVoxels + 7) / 8;
        else if (m_bitsPerVoxel == 2) newBytes = (TotalVoxels + 3) / 4;
        else if (m_bitsPerVoxel == 4) newBytes = (TotalVoxels + 1) / 2;
        else if (m_bitsPerVoxel == 8) newBytes = TotalVoxels;

        m_packedData.assign(newBytes, 0);
        for (size_t i = 0; i < TotalVoxels; ++i) {
            setRawIndex(i, oldIndices[i]);
        }
    }

    uint8_t m_bitsPerVoxel = 0;
    std::vector<BlockId> m_palette;
    std::vector<uint8_t> m_packedData;
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
