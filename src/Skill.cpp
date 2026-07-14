#include "engine/Skill.hpp"

#include <cstdio>

#include "engine/Renderer.hpp"

namespace ty {

// --- SkillBar ---------------------------------------------------------------

Skill& SkillBar::set(SkillSlot slot, Skill skill) {
    SlotState& state = slots_[index(slot)];
    state.skill = std::move(skill);
    state.cooldownLeft = 0.0f;
    return state.skill;
}

void SkillBar::setKey(SkillSlot slot, Key key) {
    int i = index(slot);
    if (i >= 0 && i < 4) keys_[i] = key;
}

Key SkillBar::key(SkillSlot slot) const {
    int i = index(slot);
    return (i >= 0 && i < 4) ? keys_[i] : Key::Count;
}

void SkillBar::update(const Input& input, float dt, Vec2 aimWorld, bool castMouse) {
    for (auto& state : slots_) {
        state.cooldownLeft -= dt;
        if (state.cooldownLeft < 0.0f) state.cooldownLeft = 0.0f;
    }

    auto slotActive = [&](int i, bool held) {
        if (i < 4) return held ? input.isDown(keys_[i]) : input.wasPressed(keys_[i]);
        MouseButton b = (i == index(SkillSlot::MouseLeft)) ? MouseButton::Left
                                                           : MouseButton::Right;
        return held ? input.isMouseDown(b) : input.wasMousePressed(b);
    };

    for (int i = 0; i < SkillSlotCount; ++i) {
        const Skill& skill = slots_[i].skill;
        if (skill.empty()) continue;
        if (i >= 4 && !castMouse) continue;
        if (slotActive(i, skill.repeatWhileHeld)) {
            tryCast(static_cast<SkillSlot>(i), aimWorld);
        }
    }
}

bool SkillBar::tryCast(SkillSlot slot, Vec2 targetWorld) {
    SlotState& state = slots_[index(slot)];
    if (state.skill.empty() || state.cooldownLeft > 0.0f) return false;
    if (state.skill.canCast && !state.skill.canCast()) return false;
    state.cooldownLeft = state.skill.cooldown;
    state.skill.onCast(targetWorld);
    return true;
}

float SkillBar::cooldownFraction(SkillSlot slot) const {
    const SlotState& state = slots_[index(slot)];
    if (state.skill.cooldown <= 0.0f) return 0.0f;
    return clamp(state.cooldownLeft / state.skill.cooldown, 0.0f, 1.0f);
}

const char* SkillBar::bindingLabel(SkillSlot slot) const {
    switch (slot) {
        case SkillSlot::MouseLeft:  return "LMB";
        case SkillSlot::MouseRight: return "RMB";
        default: break;
    }
    switch (key(slot)) {
        case Key::Q: return "Q";
        case Key::W: return "W";
        case Key::E: return "E";
        case Key::R: return "R";
        case Key::F: return "F";
        case Key::I: return "I";
        case Key::A: return "A";
        case Key::S: return "S";
        case Key::D: return "D";
        case Key::Space: return "SPC";
        case Key::Num1: return "1";
        case Key::Num2: return "2";
        case Key::Num3: return "3";
        case Key::Num4: return "4";
        case Key::Num5: return "5";
        default: return "?";
    }
}

// --- SkillBarUI ---------------------------------------------------------------

Rect SkillBarUI::slotRect(SkillSlot slot, int screenW, int screenH) const {
    const float s = style.slotSize;
    const float totalW = 6.0f * s + 4.0f * style.gap + style.groupGap;
    const float x0 = (static_cast<float>(screenW) - totalW) * 0.5f;
    const float y = static_cast<float>(screenH) - style.bottomMargin - s;

    int i = static_cast<int>(slot);
    float x = x0 + i * (s + style.gap);
    if (i >= 4) x += style.groupGap - style.gap; // mouse group sits apart
    return {x, y, s, s};
}

bool SkillBarUI::contains(Vec2 screenPos, int screenW, int screenH) const {
    for (int i = 0; i < SkillSlotCount; ++i) {
        if (slotRect(static_cast<SkillSlot>(i), screenW, screenH).contains(screenPos)) {
            return true;
        }
    }
    return false;
}

void SkillBarUI::render(Renderer& renderer, const SkillBar& bar, const Input& input,
                        int screenW, int screenH, int layer) const {
    Vec2i mouse = input.mousePosition();
    Vec2 mousePos{static_cast<float>(mouse.x), static_cast<float>(mouse.y)};
    int hovered = -1;

    for (int i = 0; i < SkillSlotCount; ++i) {
        auto slot = static_cast<SkillSlot>(i);
        Rect r = slotRect(slot, screenW, screenH);
        const Skill& skill = bar.skill(slot);
        bool hover = r.contains(mousePos);
        if (hover) hovered = i;

        // Border (2px frame) behind the box.
        renderer.drawFilledRect({r.x - 2, r.y - 2, r.w + 4, r.h + 4},
                                hover ? style.slotBorderHover : style.slotBorder, layer);
        renderer.drawFilledRect(r, style.slotBack, layer + 1);

        // Icon block: the skill's color, or a dim inlay for empty slots.
        renderer.drawFilledRect({r.x + 4, r.y + 4, r.w - 8, r.h - 8},
                                skill.empty() ? style.emptyIcon : skill.iconColor, layer + 2);

        // Cooldown: shade drains top-down, seconds left in the middle.
        float cd = bar.cooldownFraction(slot);
        if (cd > 0.0f) {
            renderer.drawFilledRect({r.x, r.y, r.w, r.h * cd}, style.cooldownShade, layer + 3);
            char text[16];
            float left = bar.cooldownRemaining(slot);
            std::snprintf(text, sizeof(text), left >= 9.95f ? "%.0f" : "%.1f", left);
            Vec2 size = renderer.measureText(text, 2.0f);
            renderer.drawText(text, {r.center().x - size.x * 0.5f, r.center().y - size.y * 0.5f},
                              2.0f, {255, 255, 255, 255}, layer + 4);
        }

        // Keycap, top-left corner.
        renderer.drawText(bar.bindingLabel(slot), {r.x + 4, r.y + 4}, 1.5f,
                          style.keycapText, layer + 4);

        // Skill name under the box.
        if (!skill.empty()) {
            Vec2 size = renderer.measureText(skill.name.c_str(), 1.0f);
            renderer.drawText(skill.name.c_str(),
                              {r.center().x - size.x * 0.5f, r.y + r.h + 6}, 1.0f,
                              style.hintText, layer + 4);
        }
    }

    // Tooltip for the hovered slot: name + hint above the bar.
    if (hovered >= 0) {
        auto slot = static_cast<SkillSlot>(hovered);
        const Skill& skill = bar.skill(slot);
        if (!skill.empty()) {
            Rect r = slotRect(slot, screenW, screenH);
            Vec2 nameSize = renderer.measureText(skill.name.c_str(), 2.0f);
            Vec2 hintSize = renderer.measureText(skill.hint.c_str(), 1.5f);
            float w = (nameSize.x > hintSize.x ? nameSize.x : hintSize.x) + 16.0f;
            float h = nameSize.y + (skill.hint.empty() ? 0.0f : hintSize.y + 6.0f) + 12.0f;
            float x = clamp(r.center().x - w * 0.5f, 4.0f, static_cast<float>(screenW) - w - 4.0f);
            float y = r.y - h - 10.0f;

            renderer.drawFilledRect({x, y, w, h}, {15, 15, 20, 235}, layer + 5);
            renderer.drawText(skill.name.c_str(), {x + 8, y + 6}, 2.0f,
                              style.nameText, layer + 6);
            if (!skill.hint.empty()) {
                renderer.drawText(skill.hint.c_str(), {x + 8, y + 6 + nameSize.y + 6}, 1.5f,
                                  style.hintText, layer + 6);
            }
        }
    }
}

} // namespace ty
