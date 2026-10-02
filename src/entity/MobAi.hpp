#pragma once
#include <cstdint>

namespace vox {

enum class MobType : uint8_t {
    Pig,
    Cow,
    PigmanVillager,
    Archivist,
    WoolWeaver,
};

// ---------------------------------------------------------------------------
// AI goals.
// ---------------------------------------------------------------------------
enum AiGoal : uint32_t {
    AiLookAt    = 1u << 0,  // idle mobs track the player
    AiFloat     = 1u << 1,  // buoyancy / swimming
    AiStepUp    = 1u << 2,  // walk up 1-block ledges without jumping
    AiAvoidEdge = 1u << 3,  // refuse to walk off a cliff
    AiJump      = 1u << 4,  // launch over taller obstacles
    AiIdle      = 1u << 5,  // stand still between decisions
    AiWander    = 1u << 6,  // pick a heading and walk it
    AiPanic     = 1u << 7,  // flee after being hit
    AiRetaliate = 1u << 8,  // fight back when attacked
    AiHomeLeash = 1u << 9,  // return to home village when outside leash radius
    AiStalk     = 1u << 10, // freeze when observed in frustum, shadow sprint when unobserved
    AiSeekLight = 1u << 11, // hunt and extinguish light sources
};

using AiGoalMask = uint32_t;

struct AiProfile {
    AiGoalMask goals = 0;
    bool has(AiGoal goal) const { return (goals & static_cast<AiGoalMask>(goal)) != 0; }
};

// Behavioural tuning shared by every mob, kept next to the profile table so a
// single read explains how a mob decides to move.
inline constexpr float kStepHeight     = 1.05f; // vertical offset that clears a 1-block ledge (if step-up enabled)
inline constexpr float kJumpSpeed      = 7.5f;  // upward launch speed, clears ~1.17 blocks with natural gravity arc
inline constexpr float kJumpCooldown   = 0.35f; // minimum time between jumps
inline constexpr float kBlockedJump    = 0.30f; // blocked this long -> jump once
inline constexpr float kBlockedReroll  = 1.40f; // blocked this long -> pick a new heading
inline constexpr float kEdgeLookahead  = 1.30f; // how far ahead to probe for a ledge
inline constexpr int   kSafeDrop       = 3;     // blocks of drop tolerated before turning away
inline constexpr float kWanderTurnRate  = 150.0f;
inline constexpr float kPanicTurnRate   = 360.0f;
inline constexpr float kHostileTurnRate = 420.0f;

// Per-mob-type goal sets.
inline constexpr AiGoalMask kPigGoals =
    AiIdle | AiWander | AiPanic | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline constexpr AiGoalMask kCowGoals =
    AiIdle | AiWander | AiPanic | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline constexpr AiGoalMask kPigmanVillagerGoals =
    AiIdle | AiWander | AiRetaliate | AiHomeLeash | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline constexpr AiGoalMask kArchivistGoals =
    AiStalk | AiJump | AiFloat;

inline constexpr AiGoalMask kWoolWeaverGoals =
    AiSeekLight | AiWander | AiJump | AiFloat;

inline AiProfile aiProfileFor(MobType type) {
    switch (type) {
        case MobType::Archivist:      return AiProfile{kArchivistGoals};
        case MobType::WoolWeaver:     return AiProfile{kWoolWeaverGoals};
        case MobType::PigmanVillager: return AiProfile{kPigmanVillagerGoals};
        case MobType::Cow:            return AiProfile{kCowGoals};
        case MobType::Pig:
        default:                      return AiProfile{kPigGoals};
    }
}

} // namespace vox
