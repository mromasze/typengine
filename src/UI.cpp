#include "engine/UI.hpp"

#include "engine/Input.hpp"

namespace ty {

UIButton& UIPanel::addButton(std::string name, Rect rect, std::function<void()> onClick) {
    UIButton btn;
    btn.name = std::move(name);
    btn.rect = rect;
    btn.onClick = std::move(onClick);
    buttons_.push_back(std::move(btn));
    return buttons_.back();
}

void UIPanel::update(const Input& input) {
    if (!visible_) return;

    Vec2i mouse = input.mousePosition();
    Vec2 m = {static_cast<float>(mouse.x), static_cast<float>(mouse.y)};
    bool released = input.wasMouseReleased(MouseButton::Left);

    for (auto& btn : buttons_) {
        btn.hovered = btn.rect.contains(m);
        if (btn.hovered && released && btn.onClick) {
            btn.onClick();
        }
    }
}

UILabel& UIPanel::addLabel(std::string text, Vec2 pos, float scale, Color color) {
    labels_.push_back({std::move(text), pos, scale, color});
    return labels_.back();
}

UIBar& UIPanel::addBar(Rect rect, float value) {
    UIBar bar;
    bar.rect = rect;
    bar.value = value;
    bars_.push_back(bar);
    return bars_.back();
}

void UIPanel::render(Renderer& renderer, int layer) const {
    if (!visible_) return;
    for (const auto& btn : buttons_) {
        renderer.drawFilledRect(btn.rect, btn.hovered ? btn.hoverColor : btn.color, layer);
        if (!btn.text.empty()) {
            Vec2 size = renderer.measureText(btn.text.c_str(), btn.textScale);
            renderer.drawText(btn.text.c_str(),
                              {btn.rect.x + (btn.rect.w - size.x) * 0.5f,
                               btn.rect.y + (btn.rect.h - size.y) * 0.5f},
                              btn.textScale, btn.textColor, layer + 1);
        }
    }
    for (const auto& label : labels_) {
        renderer.drawText(label.text.c_str(), label.pos, label.scale, label.color, layer + 1);
    }
    for (const auto& bar : bars_) {
        renderer.drawFilledRect(bar.rect, bar.back, layer);
        float v = clamp(bar.value, 0.0f, 1.0f);
        renderer.drawFilledRect(
            {bar.rect.x + 2, bar.rect.y + 2, (bar.rect.w - 4) * v, bar.rect.h - 4}, bar.fill,
            layer + 1);
    }
}

} // namespace ty
