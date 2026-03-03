#include "Typengine/ui/UISystem.h"
#include "Typengine/input/Input.h"
#include "Typengine/renderer/Renderer.h"
#include "Typengine/scripting/ScriptSystem.h"
#include <iostream>

namespace Typengine::UI {

void UISystem::AddButton(const std::string& name, float x, float y, float w, float h, const std::string& text, const std::string& callback) {
    buttons.push_back({name, text, {x, y, w, h}, callback});
}

void UISystem::Clear() {
    buttons.clear();
}

void UISystem::Update() {
    if (!isVisible) return;

    int mx, my;
    Input::Input::Get().GetMousePos(mx, my);
    bool isLeftMouseDown = Input::Input::Get().IsMouseButtonPressed(1);

    for (auto& btn : buttons) {
        bool hovered = (mx >= btn.rect.x && mx <= btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my <= btn.rect.y + btn.rect.h);
        btn.isHovered = hovered;

        if (hovered && !isLeftMouseDown && wasLeftMouseDown) {
            
            if (!btn.callbackName.empty()) {
                auto* ctx = Scripting::ScriptSystem::Get().GetContext();
                duk_get_global_string(ctx, btn.callbackName.c_str());
                if (duk_is_function(ctx, -1)) {
                    if (duk_pcall(ctx, 0) != 0) {
                        std::cerr << "[UI] Callback Error: " << duk_safe_to_string(ctx, -1) << std::endl;
                    }
                }
                duk_pop(ctx);
            }
        }
    }
    wasLeftMouseDown = isLeftMouseDown;
}

void UISystem::Render() {
    if (!isVisible) return;

    auto& renderer = Renderer::Renderer::Get();
    
    
    
    
    
    for (const auto& btn : buttons) {
        
        float colorMod = btn.isHovered ? 0.8f : 1.0f;
        
        
        
        renderer.DrawTexture(1, {0,0,1,1}, btn.rect, 10);
        
        
        
    }
}

} 
