#pragma once
#include "duktape.h"
#include <string>

namespace Typengine::Scripting {

class ScriptSystem {
public:
    static ScriptSystem& Get() { static ScriptSystem instance; return instance; }

    bool Initialize();
    void Cleanup();
    
    bool LoadScript(const std::string& path);
    
    void CallInit();
    void CallUpdate(float dt);
    void CallRender();

    duk_context* GetContext() { return ctx; }

private:
    ScriptSystem() = default;
    duk_context* ctx = nullptr;
    void BindNativeFunctions();
};

} 
