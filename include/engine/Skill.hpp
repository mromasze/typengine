#pragma once
// Typengine — Skill.hpp
// Action-RPG style skill slots: four keyboard skills (Q/E/R/F by default —
// W belongs to WASD movement) plus the two mouse buttons. The engine owns
// bindings, cooldowns and the HUD bar; games fill the slots with content:
//
//   ty::SkillBar bar;
//   bar.set(ty::SkillSlot::MouseLeft, {
//       .name = "Basic attack",
//       .hint = "Fires an arrow toward the cursor",
//       .cooldown = 0.35f,
//       .repeatWhileHeld = true,
//       .onCast = [&](ty::Vec2 target) { shootArrow(target); }});
//   // each frame, with the cursor unprojected to world space:
//   bar.update(input, dt, camera.screenToWorld(mouse));
//   skillBarUI.render(renderer, bar, input, screenW, screenH);

#include <array>
#include <functional>
#include <string>

#include "engine/Input.hpp"
#include "engine/Math.hpp"

namespace ty {

class Renderer;

enum class SkillSlot { Skill1, Skill2, Skill3, Skill4, MouseLeft, MouseRight, Count };
inline constexpr int SkillSlotCount = static_cast<int>(SkillSlot::Count);

struct Skill {
    std::string name;
    std::string hint;                // one-liner the HUD shows on hover
    float cooldown = 1.0f;           // seconds between casts
    bool repeatWhileHeld = false;    // keep casting while the binding is held
    Color iconColor{200, 200, 200, 255};

    // Called on cast with the aim point in world coordinates (the mouse
    // cursor unprojected by the game). Empty = slot stays inert.
    std::function<void(Vec2 targetWorld)> onCast;

    // Optional extra gate (resource cost, stun...). Cooldown is separate.
    std::function<bool()> canCast;

    bool empty() const { return !onCast; }
};

class SkillBar {
public:
    // Fills a slot (replacing any previous skill) and resets its cooldown.
    Skill& set(SkillSlot slot, Skill skill);
    const Skill& skill(SkillSlot slot) const { return slots_[index(slot)].skill; }

    // Rebind one of the four keyboard slots (defaults: Q, E, R, F).
    void setKey(SkillSlot slot, Key key);
    Key key(SkillSlot slot) const;

    // Ticks cooldowns and casts skills whose binding was pressed (or held,
    // for repeatWhileHeld). `aimWorld` is passed to Skill::onCast.
    // `castMouse = false` suppresses the two mouse slots for this frame
    // (pass it while the cursor is over UI, e.g. SkillBarUI::contains()).
    void update(const Input& input, float dt, Vec2 aimWorld, bool castMouse = true);

    // Manual cast, same cooldown rules. Returns true if the skill fired.
    bool tryCast(SkillSlot slot, Vec2 targetWorld);

    float cooldownRemaining(SkillSlot slot) const { return slots_[index(slot)].cooldownLeft; }
    // 1 = just cast, 0 = ready. Drives the HUD cooldown overlay.
    float cooldownFraction(SkillSlot slot) const;

    // "Q" / "E" / "R" / "F" / "LMB" / "RMB" — for the HUD keycap.
    const char* bindingLabel(SkillSlot slot) const;

private:
    static int index(SkillSlot slot) { return static_cast<int>(slot); }

    struct SlotState {
        Skill skill;
        float cooldownLeft = 0.0f;
    };
    std::array<SlotState, SkillSlotCount> slots_{};
    std::array<Key, 4> keys_{Key::Q, Key::E, Key::R, Key::F};
};

// Draws a SkillBar as a bottom-centered HUD row: four skill boxes, a small
// gap, then the LMB/RMB boxes. Shows keycaps, cooldown sweep + seconds left,
// and a name+hint tooltip for the hovered slot.
class SkillBarUI {
public:
    struct Style {
        float slotSize = 56.0f;
        float gap = 8.0f;
        float groupGap = 26.0f;      // between keyboard and mouse groups
        float bottomMargin = 18.0f;
        Color slotBack{25, 25, 32, 220};
        Color slotBorder{90, 90, 110, 255};
        Color slotBorderHover{200, 200, 120, 255};
        Color emptyIcon{45, 45, 55, 255};
        Color cooldownShade{0, 0, 0, 170};
        Color keycapText{235, 235, 235, 255};
        Color nameText{255, 220, 120, 255};
        Color hintText{220, 220, 220, 255};
    };

    Style style;

    // `input` is only used for the mouse-hover tooltip.
    void render(Renderer& renderer, const SkillBar& bar, const Input& input,
                int screenW, int screenH, int layer = 200) const;

    // Screen rect of a slot's box — for custom overlays or hit-testing.
    Rect slotRect(SkillSlot slot, int screenW, int screenH) const;

    // True when the cursor is over any slot box (games can suppress
    // world-clicks while the player interacts with the bar).
    bool contains(Vec2 screenPos, int screenW, int screenH) const;
};

} // namespace ty
