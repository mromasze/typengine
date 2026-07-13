#pragma once
// Typengine — UI.hpp
// Immediate-ish retained UI: buttons with native std::function callbacks
// (the old Duktape string-callback mechanism is gone).

#include <functional>
#include <string>
#include <vector>

#include "engine/Math.hpp"
#include "engine/Renderer.hpp"

namespace ty {

class Input;

struct UIButton {
    std::string name;
    std::string text;                 // reserved for when font rendering lands
    Rect rect{};                      // screen-space
    std::function<void()> onClick;
    Color color{60, 60, 70, 230};
    Color hoverColor{90, 90, 110, 230};
    bool hovered = false;
};

class UIPanel {
public:
    UIButton& addButton(std::string name, Rect rect, std::function<void()> onClick);
    void clear() { buttons_.clear(); }

    void setVisible(bool v) { visible_ = v; }
    bool isVisible() const { return visible_; }

    // Hover tracking + click dispatch (mouse-release inside the button).
    void update(const Input& input);
    void render(Renderer& renderer, int layer = 100) const;

private:
    std::vector<UIButton> buttons_;
    bool visible_ = false;
};

} // namespace ty
