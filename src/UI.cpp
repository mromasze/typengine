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

void UIPanel::render(Renderer& renderer, int layer) const {
    if (!visible_) return;
    for (const auto& btn : buttons_) {
        renderer.drawFilledRect(btn.rect, btn.hovered ? btn.hoverColor : btn.color, layer);
    }
}

} // namespace ty
