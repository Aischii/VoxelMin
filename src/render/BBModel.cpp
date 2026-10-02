#include "render/BBModel.hpp"
#include "core/Log.hpp"
#include "render/Texture.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

namespace vox {
namespace {

// ============================================================================
// Lightweight Zero-Dependency JSON Parser for Blockbench .bbmodel files
// ============================================================================

enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonVal {
    JsonType type = JsonType::Null;
    bool bVal = false;
    double nVal = 0.0;
    std::string sVal;
    std::vector<JsonVal> arrVal;
    std::unordered_map<std::string, JsonVal> objVal;

    bool isObject() const { return type == JsonType::Object; }
    bool isArray() const { return type == JsonType::Array; }
    bool isString() const { return type == JsonType::String; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isBool() const { return type == JsonType::Bool; }

    const JsonVal& operator[](const std::string& key) const {
        static const JsonVal nullVal;
        if (type != JsonType::Object) return nullVal;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? it->second : nullVal;
    }

    const JsonVal& operator[](size_t idx) const {
        static const JsonVal nullVal;
        if (type != JsonType::Array || idx >= arrVal.size()) return nullVal;
        return arrVal[idx];
    }

    bool has(const std::string& key) const {
        return type == JsonType::Object && objVal.find(key) != objVal.end();
    }

    std::string asString(const std::string& def = "") const {
        return (type == JsonType::String) ? sVal : def;
    }

    double asNumber(double def = 0.0) const {
        if (type == JsonType::Number) return nVal;
        if (type == JsonType::String) {
            try { return std::stod(sVal); } catch (...) {}
        }
        return def;
    }

    float asFloat(float def = 0.0f) const {
        return static_cast<float>(asNumber(def));
    }

    int asInt(int def = 0) const {
        return static_cast<int>(asNumber(def));
    }

    bool asBool(bool def = false) const {
        return (type == JsonType::Bool) ? bVal : def;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& src) : m_src(src), m_pos(0) {}

    bool parse(JsonVal& out) {
        skipWhitespace();
        return parseValue(out);
    }

private:
    const std::string& m_src;
    size_t m_pos{0};

    void skipWhitespace() {
        while (m_pos < m_src.size()) {
            char c = m_src[m_pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                m_pos++;
            } else if (c == '/' && m_pos + 1 < m_src.size() && m_src[m_pos + 1] == '/') {
                m_pos += 2;
                while (m_pos < m_src.size() && m_src[m_pos] != '\n') m_pos++;
            } else {
                break;
            }
        }
    }

    bool parseValue(JsonVal& val) {
        skipWhitespace();
        if (m_pos >= m_src.size()) return false;

        char c = m_src[m_pos];
        if (c == '{') return parseObject(val);
        if (c == '[') return parseArray(val);
        if (c == '"') return parseString(val.sVal) && (val.type = JsonType::String, true);
        if (c == 't' || c == 'f') return parseBool(val);
        if (c == 'n') return parseNull(val);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(val);
        return false;
    }

    bool parseObject(JsonVal& val) {
        val.type = JsonType::Object;
        val.objVal.clear();
        m_pos++; // skip '{'

        skipWhitespace();
        if (m_pos < m_src.size() && m_src[m_pos] == '}') {
            m_pos++;
            return true;
        }

        while (m_pos < m_src.size()) {
            skipWhitespace();
            if (m_pos >= m_src.size() || m_src[m_pos] != '"') return false;
            std::string key;
            if (!parseString(key)) return false;

            skipWhitespace();
            if (m_pos >= m_src.size() || m_src[m_pos] != ':') return false;
            m_pos++; // skip ':'

            JsonVal child;
            if (!parseValue(child)) return false;
            val.objVal[key] = std::move(child);

            skipWhitespace();
            if (m_pos < m_src.size() && m_src[m_pos] == ',') {
                m_pos++;
                skipWhitespace();
            } else if (m_pos < m_src.size() && m_src[m_pos] == '}') {
                m_pos++;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseArray(JsonVal& val) {
        val.type = JsonType::Array;
        val.arrVal.clear();
        m_pos++; // skip '['

        skipWhitespace();
        if (m_pos < m_src.size() && m_src[m_pos] == ']') {
            m_pos++;
            return true;
        }

        while (m_pos < m_src.size()) {
            JsonVal child;
            if (!parseValue(child)) return false;
            val.arrVal.push_back(std::move(child));

            skipWhitespace();
            if (m_pos < m_src.size() && m_src[m_pos] == ',') {
                m_pos++;
                skipWhitespace();
            } else if (m_pos < m_src.size() && m_src[m_pos] == ']') {
                m_pos++;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseString(std::string& str) {
        str.clear();
        m_pos++; // skip opening '"'
        while (m_pos < m_src.size()) {
            char c = m_src[m_pos++];
            if (c == '"') return true;
            if (c == '\\' && m_pos < m_src.size()) {
                char esc = m_src[m_pos++];
                if (esc == '"' || esc == '\\' || esc == '/') str += esc;
                else if (esc == 'b') str += '\b';
                else if (esc == 'f') str += '\f';
                else if (esc == 'n') str += '\n';
                else if (esc == 'r') str += '\r';
                else if (esc == 't') str += '\t';
                else str += esc;
            } else {
                str += c;
            }
        }
        return false;
    }

    bool parseNumber(JsonVal& val) {
        val.type = JsonType::Number;
        size_t start = m_pos;
        if (m_src[m_pos] == '-') m_pos++;
        while (m_pos < m_src.size() && (std::isdigit(static_cast<unsigned char>(m_src[m_pos])) || m_src[m_pos] == '.' || m_src[m_pos] == 'e' || m_src[m_pos] == 'E' || m_src[m_pos] == '+' || m_src[m_pos] == '-')) {
            m_pos++;
        }
        std::string numStr = m_src.substr(start, m_pos - start);
        try {
            val.nVal = std::stod(numStr);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool parseBool(JsonVal& val) {
        val.type = JsonType::Bool;
        if (m_src.compare(m_pos, 4, "true") == 0) {
            val.bVal = true;
            m_pos += 4;
            return true;
        }
        if (m_src.compare(m_pos, 5, "false") == 0) {
            val.bVal = false;
            m_pos += 5;
            return true;
        }
        return false;
    }

    bool parseNull(JsonVal& val) {
        val.type = JsonType::Null;
        if (m_src.compare(m_pos, 4, "null") == 0) {
            m_pos += 4;
            return true;
        }
        return false;
    }
};

glm::vec2 tileMinUV(TextureTile tile) {
    const int idx = static_cast<int>(tile);
    const float invAtlas = 1.0f / static_cast<float>(config::ATLAS_TILES);
    const int col = idx % config::ATLAS_TILES;
    const int row = idx / config::ATLAS_TILES;
    return { static_cast<float>(col) * invAtlas, static_cast<float>(row) * invAtlas };
}

constexpr glm::vec2 tileSizeUV() {
    return { 1.0f / static_cast<float>(config::ATLAS_TILES), 1.0f / static_cast<float>(config::ATLAS_TILES) };
}

TextureTile resolveTileForCube(const std::string& cubeName, TextureTile defaultTile) {
    std::string lower = cubeName;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lower.find("snout") != std::string::npos) return TextureTile::PigSnout;
    if (lower.find("horn") != std::string::npos) return TextureTile::CowHorns;
    if (lower.find("udder") != std::string::npos) return TextureTile::PigSkin;

    if (lower.find("face") != std::string::npos || lower.find("head") != std::string::npos) {
        if (defaultTile == TextureTile::PigSkin) return TextureTile::PigFace;
        if (defaultTile == TextureTile::CowSkin) return TextureTile::CowFace;
        if (defaultTile == TextureTile::PigmanSkin) return TextureTile::PigmanFace;
        if (defaultTile == TextureTile::ArchivistSkin) return TextureTile::ArchivistFace;
    }
    if (lower.find("torso") != std::string::npos || lower.find("body") != std::string::npos ||
        lower.find("chest") != std::string::npos || lower.find("cowl") != std::string::npos ||
        lower.find("pelvis") != std::string::npos || lower.find("loincloth") != std::string::npos) {
        if (defaultTile == TextureTile::PigmanSkin) return TextureTile::PigmanTorso;
        if (defaultTile == TextureTile::ArchivistSkin) return TextureTile::ArchivistTorso;
    }
    if (lower.find("hoof") != std::string::npos || (lower.find("leg") != std::string::npos && defaultTile == TextureTile::PigmanSkin)) {
        return TextureTile::PigmanHoof;
    }
    return defaultTile;
}

} // namespace

// ============================================================================
// BBAnimation Implementation
// ============================================================================

void BBAnimation::sample(float animTime, std::unordered_map<std::string, glm::mat4>& outBoneTransforms) const {
    if (length <= 0.0f) return;

    float t = animTime;
    if (loopMode == "loop") {
        t = std::fmod(animTime, length);
        if (t < 0.0f) t += length;
    } else {
        t = std::min(animTime, length);
    }

    auto interpolateKeyframes = [](const std::vector<BBKeyframe>& kfs, float evalTime) -> glm::vec3 {
        if (kfs.empty()) return glm::vec3(0.0f);
        if (kfs.size() == 1 || evalTime <= kfs.front().time) return kfs.front().value;
        if (evalTime >= kfs.back().time) return kfs.back().value;

        for (size_t i = 0; i + 1 < kfs.size(); ++i) {
            if (evalTime >= kfs[i].time && evalTime <= kfs[i + 1].time) {
                const float dt = kfs[i + 1].time - kfs[i].time;
                const float factor = (dt > 1e-5f) ? ((evalTime - kfs[i].time) / dt) : 0.0f;
                return glm::mix(kfs[i].value, kfs[i + 1].value, factor);
            }
        }
        return kfs.back().value;
    };

    for (const auto& [boneName, track] : boneTracks) {
        glm::mat4 m(1.0f);
        if (!track.positionKeyframes.empty()) {
            const glm::vec3 pos = interpolateKeyframes(track.positionKeyframes, t);
            m = glm::translate(m, pos * 0.0625f);
        }
        if (!track.rotationKeyframes.empty()) {
            const glm::vec3 rot = interpolateKeyframes(track.rotationKeyframes, t);
            m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0, 0, 1));
            m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1, 0, 0));
        }
        outBoneTransforms[boneName] = m;
    }
}

// ============================================================================
// BBModel Implementation
// ============================================================================

bool BBModel::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return loadFromString(ss.str());
}

bool BBModel::loadFromString(const std::string& jsonContent) {
    m_cubes.clear();
    m_bones.clear();
    m_animations.clear();
    m_boneNameToIndex.clear();
    m_cubeUuidToIndex.clear();

    JsonVal root;
    JsonParser parser(jsonContent);
    if (!parser.parse(root) || !root.isObject()) {
        return false;
    }

    m_name = root["name"].asString("unnamed");

    if (root.has("resolution")) {
        const JsonVal& res = root["resolution"];
        m_resolution.x = res["width"].asInt(16);
        m_resolution.y = res["height"].asInt(16);
    } else {
        m_resolution = {16, 16};
    }

    // 1. Parse Elements (Cubes)
    if (root.has("elements") && root["elements"].isArray()) {
        const JsonVal& elements = root["elements"];
        for (size_t i = 0; i < elements.arrVal.size(); ++i) {
            const JsonVal& elem = elements[i];
            BBCube cube;
            cube.name = elem["name"].asString("cube");
            cube.uuid = elem["uuid"].asString("");

            if (elem.has("from") && elem["from"].isArray()) {
                cube.from.x = elem["from"][0].asFloat(0.0f);
                cube.from.y = elem["from"][1].asFloat(0.0f);
                cube.from.z = elem["from"][2].asFloat(0.0f);
            }
            if (elem.has("to") && elem["to"].isArray()) {
                cube.to.x = elem["to"][0].asFloat(16.0f);
                cube.to.y = elem["to"][1].asFloat(16.0f);
                cube.to.z = elem["to"][2].asFloat(16.0f);
            }
            if (elem.has("origin") && elem["origin"].isArray()) {
                cube.origin.x = elem["origin"][0].asFloat(0.0f);
                cube.origin.y = elem["origin"][1].asFloat(0.0f);
                cube.origin.z = elem["origin"][2].asFloat(0.0f);
            }
            if (elem.has("rotation") && elem["rotation"].isArray()) {
                cube.rotation.x = elem["rotation"][0].asFloat(0.0f);
                cube.rotation.y = elem["rotation"][1].asFloat(0.0f);
                cube.rotation.z = elem["rotation"][2].asFloat(0.0f);
            }

            // Faces: Up(0), Down(1), North(2), South(3), West(4), East(5)
            static const char* faceKeys[6] = { "up", "down", "north", "south", "west", "east" };
            if (elem.has("faces") && elem["faces"].isObject()) {
                const JsonVal& facesObj = elem["faces"];
                for (int f = 0; f < 6; ++f) {
                    if (facesObj.has(faceKeys[f])) {
                        const JsonVal& fObj = facesObj[faceKeys[f]];
                        cube.faces[f].hasFace = true;
                        if (fObj.has("uv") && fObj["uv"].isArray()) {
                            cube.faces[f].uv.x = fObj["uv"][0].asFloat(0.0f);
                            cube.faces[f].uv.y = fObj["uv"][1].asFloat(0.0f);
                            cube.faces[f].uv.z = fObj["uv"][2].asFloat(16.0f);
                            cube.faces[f].uv.w = fObj["uv"][3].asFloat(16.0f);
                        }
                    } else {
                        cube.faces[f].hasFace = false;
                    }
                }
            }

            m_cubeUuidToIndex[cube.uuid] = m_cubes.size();
            m_cubes.push_back(std::move(cube));
        }
    }

    // 2. Parse Bones (Groups & Outliner)
    struct RawGroup {
        std::string name;
        std::string uuid;
        glm::vec3 origin{0.0f};
        glm::vec3 rotation{0.0f};
        std::vector<std::string> childUuids;
    };
    std::unordered_map<std::string, RawGroup> rawGroups;

    if (root.has("groups") && root["groups"].isArray()) {
        const JsonVal& groups = root["groups"];
        for (size_t g = 0; g < groups.arrVal.size(); ++g) {
            const JsonVal& gObj = groups[g];
            RawGroup rg;
            rg.name = gObj["name"].asString("");
            rg.uuid = gObj["uuid"].asString("");
            if (gObj.has("origin") && gObj["origin"].isArray()) {
                rg.origin.x = gObj["origin"][0].asFloat(0.0f);
                rg.origin.y = gObj["origin"][1].asFloat(0.0f);
                rg.origin.z = gObj["origin"][2].asFloat(0.0f);
            }
            if (gObj.has("rotation") && gObj["rotation"].isArray()) {
                rg.rotation.x = gObj["rotation"][0].asFloat(0.0f);
                rg.rotation.y = gObj["rotation"][1].asFloat(0.0f);
                rg.rotation.z = gObj["rotation"][2].asFloat(0.0f);
            }
            if (gObj.has("children") && gObj["children"].isArray()) {
                for (size_t c = 0; c < gObj["children"].arrVal.size(); ++c) {
                    if (gObj["children"][c].isString()) {
                        rg.childUuids.push_back(gObj["children"][c].asString());
                    }
                }
            }
            if (!rg.uuid.empty()) {
                rawGroups[rg.uuid] = rg;
            }
        }
    }

    auto parseOutlinerNode = [&](const JsonVal& node, const std::string& parentBoneName, auto& selfRef) -> void {
        if (!node.isObject()) return;
        std::string uuid = node["uuid"].asString("");
        std::string name = node["name"].asString("");
        glm::vec3 origin(0.0f);
        glm::vec3 rotation(0.0f);

        auto it = rawGroups.find(uuid);
        if (it != rawGroups.end()) {
            if (name.empty()) name = it->second.name;
            origin = it->second.origin;
            rotation = it->second.rotation;
        }

        if (node.has("origin") && node["origin"].isArray()) {
            origin.x = node["origin"][0].asFloat(0.0f);
            origin.y = node["origin"][1].asFloat(0.0f);
            origin.z = node["origin"][2].asFloat(0.0f);
        }
        if (node.has("rotation") && node["rotation"].isArray()) {
            rotation.x = node["rotation"][0].asFloat(0.0f);
            rotation.y = node["rotation"][1].asFloat(0.0f);
            rotation.z = node["rotation"][2].asFloat(0.0f);
        }

        if (name.empty()) {
            name = !uuid.empty() ? uuid : "bone";
        }

        BBBone bone;
        bone.name = name;
        bone.uuid = uuid;
        bone.parentBoneName = parentBoneName;
        bone.origin = origin;
        bone.rotation = rotation;

        if (node.has("children") && node["children"].isArray()) {
            const JsonVal& children = node["children"];
            for (size_t c = 0; c < children.arrVal.size(); ++c) {
                if (children[c].isString()) {
                    bone.childUuids.push_back(children[c].asString());
                } else if (children[c].isObject()) {
                    std::string childUuid = children[c]["uuid"].asString("");
                    std::string childName = children[c]["name"].asString("");
                    if (childName.empty()) {
                        auto cit = rawGroups.find(childUuid);
                        if (cit != rawGroups.end()) childName = cit->second.name;
                    }
                    if (!childName.empty()) {
                        bone.childBoneNames.push_back(childName);
                    }
                    selfRef(children[c], name, selfRef);
                }
            }
        }

        if (it != rawGroups.end()) {
            for (const auto& u : it->second.childUuids) {
                if (std::find(bone.childUuids.begin(), bone.childUuids.end(), u) == bone.childUuids.end()) {
                    bone.childUuids.push_back(u);
                }
            }
        }

        m_boneNameToIndex[bone.name] = m_bones.size();
        m_bones.push_back(std::move(bone));
    };

    if (root.has("outliner") && root["outliner"].isArray()) {
        const JsonVal& outliner = root["outliner"];
        for (size_t g = 0; g < outliner.arrVal.size(); ++g) {
            if (outliner[g].isObject()) {
                parseOutlinerNode(outliner[g], "", parseOutlinerNode);
            }
        }
    } else if (root.has("groups") && root["groups"].isArray()) {
        const JsonVal& groups = root["groups"];
        for (size_t g = 0; g < groups.arrVal.size(); ++g) {
            parseOutlinerNode(groups[g], "", parseOutlinerNode);
        }
    }

    // 3. Parse Animations
    if (root.has("animations") && root["animations"].isArray()) {
        const JsonVal& anims = root["animations"];
        for (size_t a = 0; a < anims.arrVal.size(); ++a) {
            const JsonVal& animObj = anims[a];
            BBAnimation anim;
            anim.name = animObj["name"].asString("anim");
            anim.length = animObj["length"].asFloat(1.0f);
            anim.loopMode = animObj["loop"].asString("loop");

            if (animObj.has("animators") && animObj["animators"].isObject()) {
                const JsonVal& animators = animObj["animators"];
                for (const auto& [animatorKey, animatorVal] : animators.objVal) {
                    std::string boneName = animatorVal["name"].asString(animatorKey);
                    BBBoneTrack track;

                    if (animatorVal.has("keyframes") && animatorVal["keyframes"].isArray()) {
                        const JsonVal& keyframes = animatorVal["keyframes"];
                        for (size_t k = 0; k < keyframes.arrVal.size(); ++k) {
                            const JsonVal& kfObj = keyframes[k];
                            BBKeyframe kf;
                            kf.time = kfObj["time"].asFloat(0.0f);
                            std::string channel = kfObj["channel"].asString("rotation");

                            if (kfObj.has("data_points") && kfObj["data_points"].isArray() && !kfObj["data_points"].arrVal.empty()) {
                                const JsonVal& dp = kfObj["data_points"][0];
                                kf.value.x = dp["x"].asFloat(0.0f);
                                kf.value.y = dp["y"].asFloat(0.0f);
                                kf.value.z = dp["z"].asFloat(0.0f);
                            }

                            if (channel == "rotation") {
                                track.rotationKeyframes.push_back(kf);
                            } else if (channel == "position") {
                                track.positionKeyframes.push_back(kf);
                            } else if (channel == "scale") {
                                track.scaleKeyframes.push_back(kf);
                            }
                        }
                    }

                    // Sort keyframes by time
                    auto sortByTime = [](BBKeyframe& k1, BBKeyframe& k2) { return k1.time < k2.time; };
                    std::sort(track.rotationKeyframes.begin(), track.rotationKeyframes.end(), sortByTime);
                    std::sort(track.positionKeyframes.begin(), track.positionKeyframes.end(), sortByTime);
                    std::sort(track.scaleKeyframes.begin(), track.scaleKeyframes.end(), sortByTime);

                    anim.boneTracks[boneName] = std::move(track);
                }
            }
            m_animations.push_back(std::move(anim));
        }
    } else if (root.has("animations") && root["animations"].isObject()) {
        const JsonVal& anims = root["animations"];
        for (const auto& [animName, animVal] : anims.objVal) {
            BBAnimation anim;
            anim.name = animName;
            anim.length = animVal["animation_length"].asFloat(1.0f);
            if (animVal.has("loop")) {
                if (animVal["loop"].isBool()) {
                    anim.loopMode = animVal["loop"].bVal ? "loop" : "once";
                } else {
                    anim.loopMode = animVal["loop"].asString("loop");
                }
            }

            if (animVal.has("bones") && animVal["bones"].isObject()) {
                const JsonVal& bones = animVal["bones"];
                for (const auto& [boneName, boneVal] : bones.objVal) {
                    BBBoneTrack track;
                    auto parseChannel = [&](const std::string& chanName, std::vector<BBKeyframe>& outKeyframes) {
                        if (!boneVal.has(chanName)) return;
                        const JsonVal& chanVal = boneVal[chanName];
                        if (chanVal.isArray()) {
                            BBKeyframe kf;
                            kf.time = 0.0f;
                            if (chanVal.arrVal.size() >= 3) {
                                kf.value.x = chanVal[0].asFloat(0.0f);
                                kf.value.y = chanVal[1].asFloat(0.0f);
                                kf.value.z = chanVal[2].asFloat(0.0f);
                            }
                            outKeyframes.push_back(kf);
                        } else if (chanVal.isObject()) {
                            for (const auto& [timeStr, vecVal] : chanVal.objVal) {
                                BBKeyframe kf;
                                kf.time = static_cast<float>(std::atof(timeStr.c_str()));
                                if (vecVal.isArray() && vecVal.arrVal.size() >= 3) {
                                    kf.value.x = vecVal[0].asFloat(0.0f);
                                    kf.value.y = vecVal[1].asFloat(0.0f);
                                    kf.value.z = vecVal[2].asFloat(0.0f);
                                }
                                outKeyframes.push_back(kf);
                            }
                        }
                    };

                    parseChannel("rotation", track.rotationKeyframes);
                    parseChannel("position", track.positionKeyframes);
                    parseChannel("scale", track.scaleKeyframes);

                    auto sortByTime = [](BBKeyframe& k1, BBKeyframe& k2) { return k1.time < k2.time; };
                    std::sort(track.rotationKeyframes.begin(), track.rotationKeyframes.end(), sortByTime);
                    std::sort(track.positionKeyframes.begin(), track.positionKeyframes.end(), sortByTime);
                    std::sort(track.scaleKeyframes.begin(), track.scaleKeyframes.end(), sortByTime);

                    anim.boneTracks[boneName] = std::move(track);
                }
            }
            m_animations.push_back(std::move(anim));
        }
    }

    matchCubesToBones();
    return !m_cubes.empty();
}

bool BBModel::loadAnimationsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return loadAnimationsFromString(content);
}

bool BBModel::loadAnimationsFromString(const std::string& jsonContent) {
    JsonParser parser(jsonContent);
    JsonVal root;
    if (!parser.parse(root)) return false;
    if (!root.has("animations")) return false;

    if (root["animations"].isArray()) {
        const JsonVal& anims = root["animations"];
        for (size_t a = 0; a < anims.arrVal.size(); ++a) {
            const JsonVal& animObj = anims[a];
            BBAnimation anim;
            anim.name = animObj["name"].asString("anim");
            anim.length = animObj["length"].asFloat(1.0f);
            anim.loopMode = animObj["loop"].asString("loop");

            if (animObj.has("animators") && animObj["animators"].isObject()) {
                const JsonVal& animators = animObj["animators"];
                for (const auto& [animatorKey, animatorVal] : animators.objVal) {
                    std::string boneName = animatorVal["name"].asString(animatorKey);
                    BBBoneTrack track;
                    if (animatorVal.has("keyframes") && animatorVal["keyframes"].isArray()) {
                        const JsonVal& keyframes = animatorVal["keyframes"];
                        for (size_t k = 0; k < keyframes.arrVal.size(); ++k) {
                            const JsonVal& kfObj = keyframes[k];
                            BBKeyframe kf;
                            kf.time = kfObj["time"].asFloat(0.0f);
                            std::string channel = kfObj["channel"].asString("rotation");

                            if (kfObj.has("data_points") && kfObj["data_points"].isArray() && !kfObj["data_points"].arrVal.empty()) {
                                const JsonVal& dp = kfObj["data_points"][0];
                                kf.value.x = dp["x"].asFloat(0.0f);
                                kf.value.y = dp["y"].asFloat(0.0f);
                                kf.value.z = dp["z"].asFloat(0.0f);
                            }

                            if (channel == "rotation") track.rotationKeyframes.push_back(kf);
                            else if (channel == "position") track.positionKeyframes.push_back(kf);
                            else if (channel == "scale") track.scaleKeyframes.push_back(kf);
                        }
                    }
                    auto sortByTime = [](BBKeyframe& k1, BBKeyframe& k2) { return k1.time < k2.time; };
                    std::sort(track.rotationKeyframes.begin(), track.rotationKeyframes.end(), sortByTime);
                    std::sort(track.positionKeyframes.begin(), track.positionKeyframes.end(), sortByTime);
                    std::sort(track.scaleKeyframes.begin(), track.scaleKeyframes.end(), sortByTime);
                    anim.boneTracks[boneName] = std::move(track);
                }
            }
            m_animations.push_back(std::move(anim));
        }
    } else if (root["animations"].isObject()) {
        const JsonVal& anims = root["animations"];
        for (const auto& [animName, animVal] : anims.objVal) {
            BBAnimation anim;
            anim.name = animName;
            anim.length = animVal["animation_length"].asFloat(1.0f);
            if (animVal.has("loop")) {
                if (animVal["loop"].isBool()) {
                    anim.loopMode = animVal["loop"].bVal ? "loop" : "once";
                } else {
                    anim.loopMode = animVal["loop"].asString("loop");
                }
            }

            if (animVal.has("bones") && animVal["bones"].isObject()) {
                const JsonVal& bones = animVal["bones"];
                for (const auto& [boneName, boneVal] : bones.objVal) {
                    BBBoneTrack track;
                    auto parseChannel = [&](const std::string& chanName, std::vector<BBKeyframe>& outKeyframes) {
                        if (!boneVal.has(chanName)) return;
                        const JsonVal& chanVal = boneVal[chanName];
                        if (chanVal.isArray()) {
                            BBKeyframe kf;
                            kf.time = 0.0f;
                            if (chanVal.arrVal.size() >= 3) {
                                kf.value.x = chanVal[0].asFloat(0.0f);
                                kf.value.y = chanVal[1].asFloat(0.0f);
                                kf.value.z = chanVal[2].asFloat(0.0f);
                            }
                            outKeyframes.push_back(kf);
                        } else if (chanVal.isObject()) {
                            for (const auto& [timeStr, vecVal] : chanVal.objVal) {
                                BBKeyframe kf;
                                kf.time = static_cast<float>(std::atof(timeStr.c_str()));
                                if (vecVal.isArray() && vecVal.arrVal.size() >= 3) {
                                    kf.value.x = vecVal[0].asFloat(0.0f);
                                    kf.value.y = vecVal[1].asFloat(0.0f);
                                    kf.value.z = vecVal[2].asFloat(0.0f);
                                }
                                outKeyframes.push_back(kf);
                            }
                        }
                    };

                    parseChannel("rotation", track.rotationKeyframes);
                    parseChannel("position", track.positionKeyframes);
                    parseChannel("scale", track.scaleKeyframes);

                    auto sortByTime = [](BBKeyframe& k1, BBKeyframe& k2) { return k1.time < k2.time; };
                    std::sort(track.rotationKeyframes.begin(), track.rotationKeyframes.end(), sortByTime);
                    std::sort(track.positionKeyframes.begin(), track.positionKeyframes.end(), sortByTime);
                    std::sort(track.scaleKeyframes.begin(), track.scaleKeyframes.end(), sortByTime);

                    anim.boneTracks[boneName] = std::move(track);
                }
            }
            m_animations.push_back(std::move(anim));
        }
    }
    return true;
}

void BBModel::matchCubesToBones() {
    for (size_t bIdx = 0; bIdx < m_bones.size(); ++bIdx) {
        BBBone& bone = m_bones[bIdx];
        bone.cubeIndices.clear();
        for (const std::string& childUuid : bone.childUuids) {
            auto it = m_cubeUuidToIndex.find(childUuid);
            if (it != m_cubeUuidToIndex.end()) {
                bone.cubeIndices.push_back(it->second);
                m_cubes[it->second].parentBone = bone.name;
            }
        }
    }
}

const BBBone* BBModel::findBone(const std::string& boneName) const {
    auto it = m_boneNameToIndex.find(boneName);
    if (it != m_boneNameToIndex.end() && it->second < m_bones.size()) {
        return &m_bones[it->second];
    }
    for (const auto& b : m_bones) {
        if (b.name == boneName) return &b;
    }
    return nullptr;
}

const BBAnimation* BBModel::findAnimation(const std::string& animName) const {
    for (const auto& anim : m_animations) {
        if (anim.name == animName || anim.name.find(animName) != std::string::npos) {
            return &anim;
        }
    }
    return nullptr;
}

void BBModel::appendGeometry(std::vector<Vertex>& vertices,
                             const glm::mat4& rootTransform,
                             TextureTile defaultTile,
                             float light,
                             float torchLight,
                             float ao,
                             float scaleFactor) const {
    static const std::unordered_map<std::string, glm::mat4> emptyBoneTransforms;
    appendAnimatedGeometry(vertices, rootTransform, emptyBoneTransforms, defaultTile, light, torchLight, ao, scaleFactor);
}

void BBModel::appendAnimatedGeometry(std::vector<Vertex>& vertices,
                                     const glm::mat4& rootTransform,
                                     const std::unordered_map<std::string, glm::mat4>& boneTransforms,
                                     TextureTile defaultTile,
                                     float light,
                                     float torchLight,
                                     float ao,
                                     float scaleFactor) const {
    // 1. Compute Hierarchical Forward Kinematics bone matrices
    std::unordered_map<std::string, glm::mat4> finalBoneMatrices;

    auto evalBoneHierarchy = [&](const BBBone& bone, const glm::mat4& parentMat, auto& selfRef) -> void {
        const glm::vec3 pivot = bone.origin * scaleFactor;
        glm::mat4 localRot(1.0f);
        auto it = boneTransforms.find(bone.name);
        if (it != boneTransforms.end()) {
            localRot = it->second;
        }
        const glm::mat4 localBoneMat = glm::translate(glm::mat4(1.0f), pivot) * localRot * glm::translate(glm::mat4(1.0f), -pivot);
        const glm::mat4 worldBoneMat = parentMat * localBoneMat;
        finalBoneMatrices[bone.name] = worldBoneMat;

        for (const auto& childName : bone.childBoneNames) {
            const BBBone* childBone = findBone(childName);
            if (childBone) {
                selfRef(*childBone, worldBoneMat, selfRef);
            }
        }
    };

    for (const auto& bone : m_bones) {
        if (bone.parentBoneName.empty()) {
            evalBoneHierarchy(bone, glm::mat4(1.0f), evalBoneHierarchy);
        }
    }
    for (const auto& bone : m_bones) {
        if (finalBoneMatrices.find(bone.name) == finalBoneMatrices.end()) {
            const glm::vec3 pivot = bone.origin * scaleFactor;
            glm::mat4 localRot(1.0f);
            auto it = boneTransforms.find(bone.name);
            if (it != boneTransforms.end()) {
                localRot = it->second;
            }
            finalBoneMatrices[bone.name] = glm::translate(glm::mat4(1.0f), pivot) * localRot * glm::translate(glm::mat4(1.0f), -pivot);
        }
    }

    const bool hasCustomTexture = (m_textureTile != TextureTile::Count);
    const glm::vec2 defaultTMin = tileMinUV(hasCustomTexture ? m_textureTile : defaultTile);
    const glm::vec2 defaultTSize = hasCustomTexture
        ? glm::vec2(static_cast<float>(m_resolution.x) / static_cast<float>(config::ATLAS_PIXELS),
                    static_cast<float>(m_resolution.y) / static_cast<float>(config::ATLAS_PIXELS))
        : tileSizeUV();

    const float resW = static_cast<float>(m_resolution.x);
    const float resH = static_cast<float>(m_resolution.y);

    for (const BBCube& cube : m_cubes) {
        glm::vec2 tMin = defaultTMin;
        glm::vec2 tSize = defaultTSize;

        if (!hasCustomTexture) {
            const TextureTile cubeTile = resolveTileForCube(cube.name, defaultTile);
            tMin = tileMinUV(cubeTile);
            tSize = tileSizeUV();
        }

        glm::mat4 boneMat(1.0f);
        if (!cube.parentBone.empty()) {
            auto bIt = finalBoneMatrices.find(cube.parentBone);
            if (bIt != finalBoneMatrices.end()) {
                boneMat = bIt->second;
            }
        }

        const glm::mat4 elemMat = rootTransform * boneMat;

        const glm::vec3 pMin = cube.from * scaleFactor;
        const glm::vec3 pMax = cube.to * scaleFactor;

        // 8 Corners
        const glm::vec3 v000(pMin.x, pMin.y, pMin.z);
        const glm::vec3 v100(pMax.x, pMin.y, pMin.z);
        const glm::vec3 v110(pMax.x, pMax.y, pMin.z);
        const glm::vec3 v010(pMin.x, pMax.y, pMin.z);
        const glm::vec3 v001(pMin.x, pMin.y, pMax.z);
        const glm::vec3 v101(pMax.x, pMin.y, pMax.z);
        const glm::vec3 v111(pMax.x, pMax.y, pMax.z);
        const glm::vec3 v011(pMin.x, pMax.y, pMax.z);

        struct FaceGeom {
            glm::vec3 normal;
            glm::vec3 c0, c1, c2, c3;
            int faceIndex;
        };

        const FaceGeom faces[6] = {
            // Up (+Y)
            { { 0,  1,  0}, v011, v111, v110, v010, 0 },
            // Down (-Y)
            { { 0, -1,  0}, v000, v100, v101, v001, 1 },
            // North (-Z)
            { { 0,  0, -1}, v100, v000, v010, v110, 2 },
            // South (+Z)
            { { 0,  0,  1}, v001, v101, v111, v011, 3 },
            // West (-X)
            { {-1,  0,  0}, v000, v001, v011, v010, 4 },
            // East (+X)
            { { 1,  0,  0}, v101, v100, v110, v111, 5 },
        };

        const glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(elemMat)));

        for (const auto& f : faces) {
            const auto& face = cube.faces[f.faceIndex];
            if (!face.hasFace) continue;

            const glm::vec3 worldNorm = glm::normalize(normMat * f.normal);
            const glm::vec3 p0 = glm::vec3(elemMat * glm::vec4(f.c0, 1.0f));
            const glm::vec3 p1 = glm::vec3(elemMat * glm::vec4(f.c1, 1.0f));
            const glm::vec3 p2 = glm::vec3(elemMat * glm::vec4(f.c2, 1.0f));
            const glm::vec3 p3 = glm::vec3(elemMat * glm::vec4(f.c3, 1.0f));

            glm::vec2 uv0(0.0f, 0.0f);
            glm::vec2 uv1(1.0f, 0.0f);
            glm::vec2 uv2(1.0f, 1.0f);
            glm::vec2 uv3(0.0f, 1.0f);

            if (hasCustomTexture && resW > 0.0f && resH > 0.0f) {
                const float u1 = face.uv.x;
                const float v1 = face.uv.y;
                const float u2 = face.uv.z;
                const float v2 = face.uv.w;

                const glm::vec2 uvTL(u1 / resW, 1.0f - v1 / resH);
                const glm::vec2 uvTR(u2 / resW, 1.0f - v1 / resH);
                const glm::vec2 uvBR(u2 / resW, 1.0f - v2 / resH);
                const glm::vec2 uvBL(u1 / resW, 1.0f - v2 / resH);

                switch (f.faceIndex) {
                    case 0: // Up (+Y)
                        uv0 = uvBL; uv1 = uvBR; uv2 = uvTR; uv3 = uvTL;
                        break;
                    case 1: // Down (-Y)
                        uv0 = uvTL; uv1 = uvTR; uv2 = uvBR; uv3 = uvBL;
                        break;
                    case 2: // North (-Z)
                        uv0 = uvBR; uv1 = uvBL; uv2 = uvTL; uv3 = uvTR;
                        break;
                    case 3: // South (+Z)
                        uv0 = uvBL; uv1 = uvBR; uv2 = uvTR; uv3 = uvTL;
                        break;
                    case 4: // West (-X)
                        uv0 = uvBR; uv1 = uvBL; uv2 = uvTL; uv3 = uvTR;
                        break;
                    case 5: // East (+X)
                        uv0 = uvBR; uv1 = uvBL; uv2 = uvTL; uv3 = uvTR;
                        break;
                }
            }

            // Two CCW triangles
            vertices.push_back({ p0, worldNorm, uv0, tMin, tSize, ao, light, torchLight });
            vertices.push_back({ p1, worldNorm, uv1, tMin, tSize, ao, light, torchLight });
            vertices.push_back({ p2, worldNorm, uv2, tMin, tSize, ao, light, torchLight });

            vertices.push_back({ p0, worldNorm, uv0, tMin, tSize, ao, light, torchLight });
            vertices.push_back({ p2, worldNorm, uv2, tMin, tSize, ao, light, torchLight });
            vertices.push_back({ p3, worldNorm, uv3, tMin, tSize, ao, light, torchLight });
        }
    }
}

// ============================================================================
// BBModelManager Implementation
// ============================================================================

BBModelManager& BBModelManager::instance() {
    static BBModelManager mgr;
    return mgr;
}

void BBModelManager::init() {
    if (m_initialized) return;
    m_initialized = true;

    // List of model search base prefixes
    const std::vector<std::string> basePaths = {
        "assets/models/",
        "../assets/models/",
        "bin/assets/models/"
    };

    auto tryLoadModel = [&](const std::string& relPath, const std::string& modelName) -> bool {
        for (const auto& base : basePaths) {
            std::string fullPath = base + relPath;
            BBModel model;
            if (model.loadFromFile(fullPath)) {
                std::string stem = fullPath;
                size_t extPos = stem.find(".bbmodel");
                if (extPos != std::string::npos) {
                    stem.replace(extPos, 8, ".json");
                    model.loadAnimationsFromFile(stem);
                }
                m_models[modelName] = std::move(model);
                return true;
            }
        }
        return false;
    };

    // 1. Mob Models
    tryLoadModel("mobs/archivist.bbmodel", "mob_archivist");
    tryLoadModel("mobs/wool_weaver.bbmodel", "mob_wool_weaver");
    tryLoadModel("mobs/pig.bbmodel", "mob_pig");
    tryLoadModel("mobs/cow.bbmodel", "mob_cow");
    tryLoadModel("mobs/pigman_villager.bbmodel", "mob_pigman_villager");

    // Assign custom texture atlas regions
    if (auto it = m_models.find("mob_archivist"); it != m_models.end()) {
        it->second.setTextureTile(TextureTile::ArchivistAtlas);
    }

    // Map Mobs
    m_mobModels[MobType::Archivist] = getModel("mob_archivist");
    m_mobModels[MobType::WoolWeaver] = getModel("mob_wool_weaver");
    m_mobModels[MobType::Pig] = getModel("mob_pig");
    m_mobModels[MobType::Cow] = getModel("mob_cow");
    m_mobModels[MobType::PigmanVillager] = getModel("mob_pigman_villager");

    // 2. Item Models
    struct ItemMapping {
        BlockId id;
        const char* file;
        const char* name;
    };

    static const ItemMapping itemMappings[] = {
        { BlockId::Bread, "items/bread.bbmodel", "item_bread" },
        { BlockId::Apple, "items/apple.bbmodel", "item_apple" },
        { BlockId::RawPorkchop, "items/raw_porkchop.bbmodel", "item_raw_porkchop" },
        { BlockId::CookedPorkchop, "items/cooked_porkchop.bbmodel", "item_cooked_porkchop" },
        { BlockId::RawBeef, "items/raw_beef.bbmodel", "item_raw_beef" },
        { BlockId::CookedBeef, "items/cooked_beef.bbmodel", "item_cooked_beef" },
        { BlockId::CondensationFlask, "items/condensation_flask.bbmodel", "item_condensation_flask" },
        { BlockId::Stick, "items/stick.bbmodel", "item_stick" },
        { BlockId::Coal, "items/coal.bbmodel", "item_coal" },
        { BlockId::IronIngot, "items/iron_ingot.bbmodel", "item_iron_ingot" },
        { BlockId::Diamond, "items/diamond.bbmodel", "item_diamond" },
        { BlockId::WoodPickaxe, "items/wood_pickaxe.bbmodel", "item_wood_pickaxe" },
        { BlockId::StonePickaxe, "items/stone_pickaxe.bbmodel", "item_stone_pickaxe" },
        { BlockId::IronPickaxe, "items/iron_pickaxe.bbmodel", "item_iron_pickaxe" },
        { BlockId::DiamondPickaxe, "items/diamond_pickaxe.bbmodel", "item_diamond_pickaxe" },
        { BlockId::WoodAxe, "items/wood_axe.bbmodel", "item_wood_axe" },
        { BlockId::StoneAxe, "items/stone_axe.bbmodel", "item_stone_axe" },
        { BlockId::IronAxe, "items/iron_axe.bbmodel", "item_iron_axe" },
        { BlockId::DiamondAxe, "items/diamond_axe.bbmodel", "item_diamond_axe" },
        { BlockId::WoodShovel, "items/wood_shovel.bbmodel", "item_wood_shovel" },
        { BlockId::StoneShovel, "items/stone_shovel.bbmodel", "item_stone_shovel" },
        { BlockId::IronShovel, "items/iron_shovel.bbmodel", "item_iron_shovel" },
        { BlockId::DiamondShovel, "items/diamond_shovel.bbmodel", "item_diamond_shovel" },
        { BlockId::WoodSword, "items/wood_sword.bbmodel", "item_wood_sword" },
        { BlockId::StoneSword, "items/stone_sword.bbmodel", "item_stone_sword" },
        { BlockId::IronSword, "items/iron_sword.bbmodel", "item_iron_sword" },
        { BlockId::DiamondSword, "items/diamond_sword.bbmodel", "item_diamond_sword" },
        { BlockId::SpawnEggPig, "items/spawn_egg_pig.bbmodel", "item_spawn_egg_pig" },
        { BlockId::SpawnEggCow, "items/spawn_egg_cow.bbmodel", "item_spawn_egg_cow" },
        { BlockId::SpawnEggPigmanVillager, "items/spawn_egg_pigman_villager.bbmodel", "item_spawn_egg_pigman_villager" },
        { BlockId::SpawnEggArchivist, "items/spawn_egg_archivist.bbmodel", "item_spawn_egg_archivist" },
        { BlockId::SpawnEggWoolWeaver, "items/spawn_egg_wool_weaver.bbmodel", "item_spawn_egg_wool_weaver" },
    };

    for (const auto& item : itemMappings) {
        if (tryLoadModel(item.file, item.name)) {
            m_itemModels[item.id] = getModel(item.name);
        }
    }

    // 3. Custom Block Models
    struct BlockMapping {
        BlockId id;
        const char* file;
        const char* name;
    };

    static const BlockMapping blockMappings[] = {
        { BlockId::OchrePlaster, "blocks/ochre_plaster.bbmodel", "block_ochre_plaster" },
        { BlockId::DampOchreWool, "blocks/damp_ochre_wool.bbmodel", "block_damp_ochre_wool" },
        { BlockId::ChiseledLimestone, "blocks/chiseled_limestone.bbmodel", "block_chiseled_limestone" },
        { BlockId::ResonantLantern, "blocks/resonant_lantern.bbmodel", "block_resonant_lantern" },
        { BlockId::FracturedBedrock, "blocks/fractured_bedrock.bbmodel", "block_fractured_bedrock" },
        { BlockId::VaultHatch, "blocks/vault_hatch.bbmodel", "block_vault_hatch" },
    };

    for (const auto& blk : blockMappings) {
        if (tryLoadModel(blk.file, blk.name)) {
            m_blockModels[blk.id] = getModel(blk.name);
        }
    }

    log::info("BBModelManager: loaded %zu Blockbench 3D models into engine", m_models.size());
}

const BBModel* BBModelManager::getModel(const std::string& name) const {
    auto it = m_models.find(name);
    return (it != m_models.end()) ? &it->second : nullptr;
}

const BBModel* BBModelManager::getMobModel(MobType type) const {
    auto it = m_mobModels.find(type);
    return (it != m_mobModels.end()) ? it->second : nullptr;
}

const BBModel* BBModelManager::getItemModel(BlockId id) const {
    auto it = m_itemModels.find(id);
    return (it != m_itemModels.end()) ? it->second : nullptr;
}

const BBModel* BBModelManager::getBlockModel(BlockId id) const {
    auto it = m_blockModels.find(id);
    return (it != m_blockModels.end()) ? it->second : nullptr;
}

bool BBModelManager::hasMobModel(MobType type) const {
    return getMobModel(type) != nullptr;
}

bool BBModelManager::hasItemModel(BlockId id) const {
    return getItemModel(id) != nullptr;
}

bool BBModelManager::hasBlockModel(BlockId id) const {
    return getBlockModel(id) != nullptr;
}

} // namespace vox
