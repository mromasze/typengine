#pragma once
#include <string>
#include <vector>
#include <functional>
#include "Typengine/renderer/Renderer.h"

namespace Typengine::UI {

struct UIButton {
    std::string name;
    std::string text;
    Renderer::Rect rect;
    std::string callbackName; 
    bool isHovered = false;
};

class UISystem {
public:
    static UISystem& Get() { static UISystem instance; return instance; }

    void Update();
    void Render();

    void AddButton(const std::string& name, float x, float y, float w, float h, const std::string& text, const std::string& callback);
    void Clear();
    
    void SetVisible(bool visible) { isVisible = visible; }
    bool IsVisible() const { return isVisible; }

private:
    UISystem() = default;
    std::vector<UIButton> buttons;
    bool isVisible = false;
    bool wasLeftMouseDown = false;
};

} 
