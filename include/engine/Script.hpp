#pragma once
// Typengine — Script.hpp (optional module, built when TYPENGINE_SCRIPTING=ON)
//
// Lua 5.4 + sol2 script host for *content* scripting: GUI, quests, dialogue.
// Design contract: scripts orchestrate, C++ executes — the VM reacts to events
// emitted by game code, it is never called per-frame per-entity.
//
// Engine API exposed to Lua (bound in initialize()):
//   engine.log(msg) / engine.time() / engine.load(file)
//   engine.on(event, [key,] fn)  -- keyed handlers survive hot-reload cleanly
//   engine.emit(event, payload) / engine.after(seconds, fn)
//   ui.panel(name) -> panel ; panel:button{...} :label{...} :bar{...}
//                     :show() :hide() :clear() :visible()
//   input.is_down(key) / input.was_pressed(key)
//
// Hot-reload: files loaded through runFile()/engine.load() are watched (mtime,
// ~1s poll) and re-run on change; write scripts idempotently (re-registering a
// keyed handler or rebuilding a named panel replaces the previous one).
// Script errors are logged and never crash the game.
//
// This header is cheap to include (sol2 forward declarations only). Only the
// translation units that *register bindings* need <sol/sol.hpp>.

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include <sol/forward.hpp>

namespace ty {

class Input;
class Renderer;

// Payload field value for the sol-free emit() overload.
using ScriptValue = std::variant<bool, int, double, std::string>;

class ScriptHost {
public:
    ScriptHost();
    ~ScriptHost();
    ScriptHost(const ScriptHost&) = delete;
    ScriptHost& operator=(const ScriptHost&) = delete;

    // Opens the VM, standard libraries and the engine.*/ui.*/input.* bindings.
    bool initialize();
    void shutdown();

    // The raw sol2 state — register your game's own API here
    // (include <sol/sol.hpp> in that translation unit).
    sol::state& lua();

    // Directory scripts are resolved against (default "scripts").
    void setScriptRoot(std::string dir);
    // Runs + tracks a file for hot-reload. Returns false on load/runtime error.
    bool runFile(const std::string& relPath);

    // Per-frame: timers, hot-reload scan, script-owned UI input. Call once.
    void update(const Input& input, float dt);
    // Draws script-created ui.* panels.
    void render(Renderer& renderer);

    // Fire an event into Lua without touching sol types:
    //   scripts.emit("enemy_killed", {{"x", 3.5}, {"remaining", 2}});
    void emit(const std::string& event,
              std::initializer_list<std::pair<const char*, ScriptValue>> fields = {});
    // Same, with a payload table you built via lua().
    void emitTable(const std::string& event, sol::table payload);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace ty
