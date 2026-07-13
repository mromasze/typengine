// typengine — scripting example
// The C++ side is a thin shell: it opens a window, loads scripts/hello.lua
// and forwards frame updates. All UI and logic live in the script.
// Edit scripts/hello.lua while this runs to see hot-reload in action.

#include <memory>

#include <engine/Engine.hpp>
#include <engine/Script.hpp>

namespace {

class ScriptedScene : public ty::Scene {
public:
    void onEnter(ty::Engine& engine) override {
        scripts.initialize();
        scripts.setScriptRoot("scripts");
        scripts.runFile("hello.lua");
        scripts.emit("game_started");
    }

    void update(ty::Engine& engine, float dt) override {
        if (engine.input().wasPressed(ty::Key::Escape)) {
            engine.quit();
            return;
        }
        scripts.update(engine.input(), dt);
    }

    void render(ty::Engine& engine) override { scripts.render(engine.renderer()); }

private:
    ty::ScriptHost scripts;
};

} // namespace

int main(int, char**) {
    ty::Engine engine;
    if (!engine.initialize({.title = "typengine — Lua scripting", .width = 960, .height = 540}))
        return 1;
    engine.run(std::make_unique<ScriptedScene>());
    return 0;
}
