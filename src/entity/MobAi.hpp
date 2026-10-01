#pragma once
#include <cstdint>

namespace vox {

enum class MobType : uint8_t {
    Pig,
    Cow,
    PigmanVillager,
};

// ---------------------------------------------------------------------------
// AI goals.
//
// Every mob behaviour is a named bit in a mask rather than an `if` buried in
// the AI update. That means a whole goal can be switched off for a mob type by
// editing one table entry below, which is the same "filter layer" idea used by
// the AI-Improvements mod (see docs/SOURCES.md, section 4).
//
// The bits are ordered cheapest-test-first (a single block probe) so the goals
// that run most often are also the ones that bail out fastest.
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

// Per-mob-type goal sets. Mobs jump naturally over 1-block ledges rather than
// teleporting with step-up.
inline constexpr AiGoalMask kPigGoals =
    AiIdle | AiWander | AiPanic | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline constexpr AiGoalMask kCowGoals =
    AiIdle | AiWander | AiPanic | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline constexpr AiGoalMask kPigmanVillagerGoals =
    AiIdle | AiWander | AiRetaliate | AiHomeLeash | AiLookAt | AiJump | AiAvoidEdge | AiFloat;

inline AiProfile aiProfileFor(MobType type) {
    switch (type) {
        case MobType::PigmanVillager: return AiProfile{kPigmanVillagerGoals};
        case MobType::Cow:            return AiProfile{kCowGoals};
        case MobType::Pig:
        default:                      return AiProfile{kPigGoals};
    }
}

} // namespace vox
