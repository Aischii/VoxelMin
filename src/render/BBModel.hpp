#pragma once
#include "core/Config.hpp"
#include "entity/MobAi.hpp"
#include "render/Mesh.hpp"
#include "world/Block.hpp"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace vox {

struct BBCubeFace {
    glm::vec4 uv{0.0f, 0.0f, 16.0f, 16.0f}; // [u1, v1, u2, v2]
    TextureTile textureTile{TextureTile::GrassTop};
    bool hasFace{true};
};

struct BBCube {
    std::string name;
    std::string uuid;
    glm::vec3 from{0.0f};     // In 1/16 voxel pixel units
    glm::vec3 to{16.0f};
    glm::vec3 origin{0.0f};   // Pivot point
    glm::vec3 rotation{0.0f}; // Rotation angles in degrees
    std::string parentBone;
    BBCubeFace faces[6];      // 0: Up(+Y), 1: Down(-Y), 2: North(-Z), 3: South(+Z), 4: West(-X), 5: East(+X)
};

struct BBBone {
    std::string name;
    std::string uuid;
    std::string parentBoneName;
    glm::vec3 origin{0.0f};   // Pivot point in voxel pixel units
    glm::vec3 rotation{0.0f}; // Default rotation in degrees
    std::vector<std::string> childUuids;
    std::vector<std::string> childBoneNames;
    std::vector<size_t> cubeIndices;
};

struct BBKeyframe {
    float time{0.0f};
    glm::vec3 value{0.0f};
};

struct BBBoneTrack {
    std::vector<BBKeyframe> positionKeyframes;
    std::vector<BBKeyframe> rotationKeyframes;
    std::vector<BBKeyframe> scaleKeyframes;
};

struct BBAnimation {
    std::string name;
    float length{1.0f};
    std::string loopMode{"loop"}; // "loop", "once", "hold_on_last_frame"
    std::unordered_map<std::string, BBBoneTrack> boneTracks;

    void sample(float time, std::unordered_map<std::string, glm::mat4>& outBoneTransforms) const;
};

class BBModel {
public:
    BBModel() = default;

    bool loadFromFile(const std::string& filepath);
    bool loadFromString(const std::string& jsonContent);
    bool loadAnimationsFromFile(const std::string& filepath);
    bool loadAnimationsFromString(const std::string& jsonContent);

    bool isValid() const { return !m_cubes.empty(); }
    const std::string& name() const { return m_name; }
    const std::vector<BBCube>& cubes() const { return m_cubes; }
    const std::vector<BBBone>& bones() const { return m_bones; }
    const std::vector<BBAnimation>& animations() const { return m_animations; }
    const glm::ivec2& resolution() const { return m_resolution; }
    TextureTile textureTile() const { return m_textureTile; }
    void setTextureTile(TextureTile tile) { m_textureTile = tile; }

    const BBBone* findBone(const std::string& boneName) const;
    const BBAnimation* findAnimation(const std::string& animName) const;

    // Renders static model scaled by scaleFactor (default 1.0f/16.0f to convert 16px to 1.0 world block)
    void appendGeometry(std::vector<Vertex>& vertices,
                        const glm::mat4& rootTransform,
                        TextureTile defaultTile,
                        float light = 1.0f,
                        float torchLight = 0.0f,
                        float ao = 1.0f,
                        float scaleFactor = 0.0625f) const;

    // Renders animated model with per-bone relative transforms
    void appendAnimatedGeometry(std::vector<Vertex>& vertices,
                                const glm::mat4& rootTransform,
                                const std::unordered_map<std::string, glm::mat4>& boneTransforms,
                                TextureTile defaultTile,
                                float light = 1.0f,
                                float torchLight = 0.0f,
                                float ao = 1.0f,
                                float scaleFactor = 0.0625f) const;

private:
    std::string m_name;
    glm::ivec2 m_resolution{16, 16};
    TextureTile m_textureTile{TextureTile::Count};
    std::vector<BBCube> m_cubes;
    std::vector<BBBone> m_bones;
    std::vector<BBAnimation> m_animations;
    std::unordered_map<std::string, size_t> m_boneNameToIndex;
    std::unordered_map<std::string, size_t> m_cubeUuidToIndex;

    void matchCubesToBones();
};

class BBModelManager {
public:
    static BBModelManager& instance();

    void init();

    const BBModel* getMobModel(MobType type) const;
    const BBModel* getItemModel(BlockId id) const;
    const BBModel* getBlockModel(BlockId id) const;
    const BBModel* getModel(const std::string& name) const;

    bool hasMobModel(MobType type) const;
    bool hasItemModel(BlockId id) const;
    bool hasBlockModel(BlockId id) const;

private:
    BBModelManager() = default;

    std::unordered_map<std::string, BBModel> m_models;
    std::unordered_map<MobType, const BBModel*> m_mobModels;
    std::unordered_map<BlockId, const BBModel*> m_itemModels;
    std::unordered_map<BlockId, const BBModel*> m_blockModels;
    bool m_initialized{false};
};

} // namespace vox
