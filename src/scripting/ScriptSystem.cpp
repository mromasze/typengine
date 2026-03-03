#include "Typengine/scripting/ScriptSystem.h"
#include "Typengine/renderer/Renderer.h"
#include "Typengine/input/Input.h"
#include "Typengine/core/Engine.h"
#include "Typengine/core/World.h"
#include "Typengine/ui/UISystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Typengine::Scripting {

using namespace Typengine::Renderer;
using namespace Typengine::Input;
using namespace Typengine::Core;



static duk_ret_t js_Log(duk_context* ctx) {
    const char* msg = duk_to_string(ctx, 0);
    std::cout << "[JS] " << msg << std::endl;
    return 0;
}

static duk_ret_t js_DrawTexture(duk_context* ctx) {
    int id = duk_require_int(ctx, 0);
    float sx = (float)duk_require_number(ctx, 1);
    float sy = (float)duk_require_number(ctx, 2);
    float sw = (float)duk_require_number(ctx, 3);
    float sh = (float)duk_require_number(ctx, 4);
    float dx = (float)duk_require_number(ctx, 5);
    float dy = (float)duk_require_number(ctx, 6);
    float dw = (float)duk_require_number(ctx, 7);
    float dh = (float)duk_require_number(ctx, 8);
    int layer = duk_require_int(ctx, 9);
    
    Renderer::Renderer::Get().DrawTexture(id, {sx, sy, sw, sh}, {dx, dy, dw, dh}, layer);
    return 0;
}

static duk_ret_t js_LoadTexture(duk_context* ctx) {
    const char* path = duk_require_string(ctx, 0);
    int id = Renderer::Renderer::Get().LoadTexture(path);
    duk_push_int(ctx, id);
    return 1;
}

static duk_ret_t js_IsKeyDown(duk_context* ctx) {
    const char* keyName = duk_require_string(ctx, 0);
    std::string k = keyName;
    bool pressed = false;
    
    if (k == "W") pressed = Input::Input::Get().IsKeyPressed(Key::W);
    else if (k == "A") pressed = Input::Input::Get().IsKeyPressed(Key::A);
    else if (k == "S") pressed = Input::Input::Get().IsKeyPressed(Key::S);
    else if (k == "D") pressed = Input::Input::Get().IsKeyPressed(Key::D);
    else if (k == "SHIFT") pressed = Input::Input::Get().IsKeyPressed(Key::SHIFT);
    else if (k == "ESC") pressed = Input::Input::Get().IsKeyPressed(Key::ESC);
    
    duk_push_boolean(ctx, pressed);
    return 1;
}

static duk_ret_t js_GetScreenWidth(duk_context* ctx) {
    duk_push_int(ctx, Engine::Get().GetScreenWidth());
    return 1;
}

static duk_ret_t js_GetScreenHeight(duk_context* ctx) {
    duk_push_int(ctx, Engine::Get().GetScreenHeight());
    return 1;
}

static duk_ret_t js_AddUIButton(duk_context* ctx) {
    const char* name = duk_require_string(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    float w = (float)duk_require_number(ctx, 3);
    float h = (float)duk_require_number(ctx, 4);
    const char* text = duk_require_string(ctx, 5);
    const char* callback = duk_require_string(ctx, 6);
    UI::UISystem::Get().AddButton(name, x, y, w, h, text, callback);
    return 0;
}

static duk_ret_t js_ClearUI(duk_context* ctx) {
    UI::UISystem::Get().Clear();
    return 0;
}

static duk_ret_t js_SetUIVisible(duk_context* ctx) {
    UI::UISystem::Get().SetVisible(duk_require_boolean(ctx, 0));
    return 0;
}

static duk_ret_t js_SetPaused(duk_context* ctx) {
    Engine::Get().SetPaused(duk_require_boolean(ctx, 0));
    return 0;
}

static duk_ret_t js_IsPaused(duk_context* ctx) {
    duk_push_boolean(ctx, Engine::Get().IsPaused());
    return 1;
}


static duk_ret_t js_DrawNumber(duk_context* ctx) {
    float num = (float)duk_require_number(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    float size = (float)duk_require_number(ctx, 3);
    int colorTex = duk_require_int(ctx, 4);

    std::string str = std::to_string((int)num); 
    
    float s = size;
    float s2 = s / 2;
    float thickness = std::max(1.0f, s / 5.0f);
    
    auto drawLine = [&](float dx, float dy, float dw, float dh) {
        Renderer::Renderer::Get().DrawTexture(colorTex, {0,0,1,1}, {dx, dy, dw, dh}, 10);
    };

    float cursorX = x;
    int segments[10][7] = {
        {1,1,1,1,1,1,0}, 
        {0,1,1,0,0,0,0}, 
        {1,1,0,1,1,0,1}, 
        {1,1,1,1,0,0,1}, 
        {0,1,1,0,0,1,1}, 
        {1,0,1,1,0,1,1}, 
        {1,0,1,1,1,1,1}, 
        {1,1,1,0,0,0,0}, 
        {1,1,1,1,1,1,1}, 
        {1,1,1,1,0,1,1}  
    };

    for(char c : str) {
        if(c < '0' || c > '9') continue;
        int digit = c - '0';
        int* seg = segments[digit];

        if (seg[0]) drawLine(cursorX, y, s, thickness);
        if (seg[1]) drawLine(cursorX + s - thickness, y, thickness, s2);
        if (seg[2]) drawLine(cursorX + s - thickness, y + s2, thickness, s2);
        if (seg[3]) drawLine(cursorX, y + s - thickness, s, thickness);
        if (seg[4]) drawLine(cursorX, y + s2, thickness, s2);
        if (seg[5]) drawLine(cursorX, y, thickness, s2);
        if (seg[6]) drawLine(cursorX, y + s2 - thickness/2, s, thickness);

        cursorX += s * 1.2f;
    }
    return 0;
}

static duk_ret_t js_SetMapDimensions(duk_context* ctx) {
    int w = duk_require_int(ctx, 0);
    int h = duk_require_int(ctx, 1);
    int ts = duk_require_int(ctx, 2);
    Core::World::Get().SetMapDimensions(w, h, ts);
    return 0;
}

static duk_ret_t js_SetMapTile(duk_context* ctx) {
    int x = duk_require_int(ctx, 0);
    int y = duk_require_int(ctx, 1);
    int value = duk_require_int(ctx, 2);
    Core::World::Get().SetMapTile(x, y, value);
    return 0;
}

static duk_ret_t js_GetMapTile(duk_context* ctx) {
    int x = duk_require_int(ctx, 0);
    int y = duk_require_int(ctx, 1);
    duk_push_int(ctx, Core::World::Get().GetMapTile(x, y));
    return 1;
}

static duk_ret_t js_AddCollisionBox(duk_context* ctx) {
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float w = (float)duk_require_number(ctx, 2);
    float h = (float)duk_require_number(ctx, 3);
    Core::World::Get().AddCollisionBox(x, y, w, h);
    return 0;
}

static duk_ret_t js_IsWalkable(duk_context* ctx) {
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float w = (float)duk_require_number(ctx, 2);
    float h = (float)duk_require_number(ctx, 3);
    duk_push_boolean(ctx, Core::World::Get().IsWalkable(x, y, w, h));
    return 1;
}




bool ScriptSystem::Initialize() {
    ctx = duk_create_heap_default();
    if (!ctx) return false;
    BindNativeFunctions();
    return true;
}

void ScriptSystem::Cleanup() {
    if (ctx) { duk_destroy_heap(ctx); ctx = nullptr; }
}

void ScriptSystem::BindNativeFunctions() {
    auto reg = [&](const char* name, duk_c_function func, int args) {
        duk_push_c_function(ctx, func, args);
        duk_put_global_string(ctx, name);
    };

    reg("Log", js_Log, 1);
    reg("DrawTexture", js_DrawTexture, 10);
    reg("LoadTexture", js_LoadTexture, 1);
    reg("IsKeyDown", js_IsKeyDown, 1);
    reg("GetScreenWidth", js_GetScreenWidth, 0);
    reg("GetScreenHeight", js_GetScreenHeight, 0);
    reg("AddUIButton", js_AddUIButton, 7);
    reg("ClearUI", js_ClearUI, 0);
    reg("SetUIVisible", js_SetUIVisible, 1);
    reg("SetPaused", js_SetPaused, 1);
    reg("IsPaused", js_IsPaused, 0);
    
    
    reg("DrawNumber", js_DrawNumber, 5);
    reg("SetMapDimensions", js_SetMapDimensions, 3);
    reg("SetMapTile", js_SetMapTile, 3);
    reg("GetMapTile", js_GetMapTile, 2);
    reg("AddCollisionBox", js_AddCollisionBox, 4);
    reg("IsWalkable", js_IsWalkable, 4);

}

bool ScriptSystem::LoadScript(const std::string& path) {
    std::ifstream t(path);
    if (!t.is_open()) {
        t.open("../" + path);
        if (!t.is_open()) return false;
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string code = buffer.str();
    
    if (duk_peval_string(ctx, code.c_str()) != 0) {
        std::cerr << "Script Error: " << duk_safe_to_string(ctx, -1) << std::endl;
        return false;
    }
    duk_pop(ctx);
    return true;
}

void ScriptSystem::CallInit() {
    duk_get_global_string(ctx, "Init");
    if (duk_is_function(ctx, -1)) {
        if (duk_pcall(ctx, 0) != 0) std::cerr << "Error in Init: " << duk_safe_to_string(ctx, -1) << std::endl;
    }
    duk_pop(ctx);
}

void ScriptSystem::CallUpdate(float dt) {
    duk_get_global_string(ctx, "Update");
    if (duk_is_function(ctx, -1)) {
        duk_push_number(ctx, dt);
        if (duk_pcall(ctx, 1) != 0) std::cerr << "Error in Update: " << duk_safe_to_string(ctx, -1) << std::endl;
        duk_pop(ctx);
    } else {
        duk_pop(ctx);
    }
}

void ScriptSystem::CallRender() {
    duk_get_global_string(ctx, "Render");
    if (duk_is_function(ctx, -1)) {
        if (duk_pcall(ctx, 0) != 0) std::cerr << "Error in Render: " << duk_safe_to_string(ctx, -1) << std::endl;
    }
    duk_pop(ctx);
}

} 
