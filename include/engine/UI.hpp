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
    std::string text;                 // drawn centered with the built-in font
    Rect rect{};                      // screen-space
    std::function<void()> onClick;
    Color color{60, 60, 70, 230};
    Color hoverColor{90, 90, 110, 230};
    Color textColor{235, 235, 235, 255};
    float textScale = 2.0f;
    bool hovered = false;
};

struct UILabel {
    std::string text;
    Vec2 pos{};
    float scale = 2.0f;
    Color color{235, 235, 235, 255};
};

struct UIBar {
    Rect rect{};
    float value = 1.0f;               // 0..1 fill fraction
    Color fill{80, 190, 90, 255};
    Color back{0, 0, 0, 180};
};

class UIPanel {
public:
    UIButton& addButton(std::string name, Rect rect, std::function<void()> onClick);
    UILabel& addLabel(std::string text, Vec2 pos, float scale = 2.0f,
                      Color color = {235, 235, 235, 255});
    UIBar& addBar(Rect rect, float value = 1.0f);
    void clear() {
        buttons_.clear();
        labels_.clear();
        bars_.clear();
    }

    void setVisible(bool v) { visible_ = v; }
    bool isVisible() const { return visible_; }

    // Hover tracking + click dispatch (mouse-release inside the button).
    void update(const Input& input);
    void render(Renderer& renderer, int layer = 100) const;

private:
    std::vector<UIButton> buttons_;
    std::vector<UILabel> labels_;
    std::vector<UIBar> bars_;
    bool visible_ = false;
};

} // namespace ty
