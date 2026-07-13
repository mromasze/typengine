#include "engine/Script.hpp"

#include <sol/sol.hpp>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "engine/Input.hpp"
#include "engine/Renderer.hpp"
#include "engine/UI.hpp"

namespace fs = std::filesystem;

namespace ty {

namespace {

// Defines engine.on/engine.off on top of a persistent handler table. Handlers
// are keyed so a re-run script replaces its handlers instead of stacking them.
constexpr const char* kBootstrap = R"lua(
engine._handlers = engine._handlers or {}
function engine.on(event, key, fn)
    if fn == nil then fn = key; key = "default" end
    local h = engine._handlers[event]
    if h == nil then h = {}; engine._handlers[event] = h end
    h[key] = fn
end
function engine.off(event, key)
    local h = engine._handlers[event]
    if h then h[key or "default"] = nil end
end
)lua";

Key keyFromName(std::string name) {
    for (auto& ch : name) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (name == "W") return Key::W;
    if (name == "A") return Key::A;
    if (name == "S") return Key::S;
    if (name == "D") return Key::D;
    if (name == "UP") return Key::Up;
    if (name == "DOWN") return Key::Down;
    if (name == "LEFT") return Key::Left;
    if (name == "RIGHT") return Key::Right;
    if (name == "SHIFT") return Key::Shift;
    if (name == "CTRL") return Key::Ctrl;
    if (name == "SPACE") return Key::Space;
    if (name == "ESC" || name == "ESCAPE") return Key::Escape;
    if (name == "ENTER" || name == "RETURN") return Key::Enter;
    if (name == "TAB") return Key::Tab;
    if (name == "E") return Key::E;
    if (name == "Q") return Key::Q;
    if (name == "F") return Key::F;
    if (name == "R") return Key::R;
    if (name == "I") return Key::I;
    if (name == "1") return Key::Num1;
    if (name == "2") return Key::Num2;
    if (name == "3") return Key::Num3;
    if (name == "4") return Key::Num4;
    if (name == "5") return Key::Num5;
    return Key::Count; // unknown
}

void logScriptError(const char* what, const sol::error& e) {
    std::cerr << "[typengine/lua] " << what << ": " << e.what() << "\n";
}

Color colorFromTable(const sol::table& t, Color fallback) {
    Color c = fallback;
    c.r = static_cast<std::uint8_t>(t.get_or("r", static_cast<int>(fallback.r)));
    c.g = static_cast<std::uint8_t>(t.get_or("g", static_cast<int>(fallback.g)));
    c.b = static_cast<std::uint8_t>(t.get_or("b", static_cast<int>(fallback.b)));
    c.a = static_cast<std::uint8_t>(t.get_or("a", static_cast<int>(fallback.a)));
    return c;
}

} // namespace

struct ScriptHost::Impl {
    sol::state lua;
    std::string root = "scripts";
    bool initialized = false;

    struct TrackedFile {
        std::string rel;
        fs::path resolved;
        fs::file_time_type mtime;
    };
    std::vector<TrackedFile> files;
    float reloadTimer = 0.0f;

    struct Timer {
        float remaining;
        sol::protected_function fn;
    };
    std::vector<Timer> timers;

    // Script-created panels; unordered_map keeps addresses stable for Lua refs.
    std::unordered_map<std::string, std::unique_ptr<UIPanel>> panels;

    const Input* input = nullptr;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    double now() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

    fs::path resolve(const std::string& rel) const {
        fs::path p = fs::path(root) / rel;
        if (fs::exists(p)) return p;
        fs::path fallback = fs::path("..") / root / rel; // match texture loading behavior
        if (fs::exists(fallback)) return fallback;
        return p;
    }

    void dispatch(const std::string& event, sol::object payload) {
        sol::object handlersObj = lua["engine"]["_handlers"][event];
        if (!handlersObj.is<sol::table>()) return;

        // Copy first: handlers may (un)register handlers while running.
        std::vector<sol::protected_function> callbacks;
        for (auto& [key, value] : handlersObj.as<sol::table>()) {
            if (value.is<sol::protected_function>())
                callbacks.push_back(value.as<sol::protected_function>());
        }
        for (auto& fn : callbacks) {
            sol::protected_function_result result = fn(payload);
            if (!result.valid()) logScriptError(("event '" + event + "'").c_str(), result);
        }
    }

    bool run(TrackedFile& file) {
        sol::protected_function_result result =
            lua.safe_script_file(file.resolved.string(), sol::script_pass_on_error);
        if (!result.valid()) {
            logScriptError(file.rel.c_str(), result);
            return false;
        }
        return true;
    }
};

ScriptHost::ScriptHost() : impl(std::make_unique<Impl>()) {}
ScriptHost::~ScriptHost() { shutdown(); }

bool ScriptHost::initialize() {
    auto& lua = impl->lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math,
                       sol::lib::table, sol::lib::os);

    // --- engine.* ------------------------------------------------------------
    sol::table engineTbl = lua.create_named_table("engine");
    engineTbl["log"] = [](const std::string& msg) { std::cout << "[lua] " << msg << std::endl; };
    engineTbl["time"] = [this] { return impl->now(); };
    engineTbl["after"] = [this](double seconds, sol::protected_function fn) {
        impl->timers.push_back({static_cast<float>(seconds), std::move(fn)});
    };
    engineTbl["load"] = [this](const std::string& rel) { return runFile(rel); };
    engineTbl["emit"] = [this](const std::string& event, sol::object payload) {
        impl->dispatch(event, payload.valid() ? payload
                                              : sol::object(impl->lua.create_table()));
    };

    sol::protected_function_result boot = lua.safe_script(kBootstrap, sol::script_pass_on_error);
    if (!boot.valid()) {
        logScriptError("bootstrap", boot);
        return false;
    }

    // --- ui.* ----------------------------------------------------------------
    lua.new_usertype<UIPanel>(
        "UIPanel", sol::no_constructor,
        "show", [](UIPanel& p) { p.setVisible(true); },
        "hide", [](UIPanel& p) { p.setVisible(false); },
        "visible", [](UIPanel& p) { return p.isVisible(); },
        "clear", [](UIPanel& p) { p.clear(); },
        "button",
        [](UIPanel& p, sol::table t) {
            Rect rect{t.get_or("x", 0.0f), t.get_or("y", 0.0f), t.get_or("w", 160.0f),
                      t.get_or("h", 40.0f)};
            std::string text = t.get_or<std::string>("text", "");
            sol::protected_function onClick = t.get_or("on_click", sol::protected_function{});
            auto& btn = p.addButton(text, rect, [onClick] {
                if (!onClick.valid()) return;
                sol::protected_function_result r = onClick();
                if (!r.valid()) logScriptError("button on_click", r);
            });
            btn.text = std::move(text);
            btn.textScale = t.get_or("text_scale", 2.0f);
            if (t["color"].is<sol::table>()) btn.color = colorFromTable(t["color"], btn.color);
        },
        "label",
        [](UIPanel& p, sol::table t) {
            auto& label = p.addLabel(t.get_or<std::string>("text", ""),
                                     {t.get_or("x", 0.0f), t.get_or("y", 0.0f)},
                                     t.get_or("scale", 2.0f));
            if (t["color"].is<sol::table>()) label.color = colorFromTable(t["color"], label.color);
        },
        "bar",
        [](UIPanel& p, sol::table t) {
            auto& bar = p.addBar({t.get_or("x", 0.0f), t.get_or("y", 0.0f),
                                  t.get_or("w", 200.0f), t.get_or("h", 16.0f)},
                                 t.get_or("value", 1.0f));
            if (t["fill"].is<sol::table>()) bar.fill = colorFromTable(t["fill"], bar.fill);
        });

    sol::table uiTbl = lua.create_named_table("ui");
    uiTbl["panel"] = [this](const std::string& name) -> UIPanel& {
        auto& slot = impl->panels[name];
        if (!slot) slot = std::make_unique<UIPanel>();
        return *slot;
    };

    // --- input.* ---------------------------------------------------------------
    sol::table inputTbl = lua.create_named_table("input");
    inputTbl["is_down"] = [this](const std::string& name) {
        Key k = keyFromName(name);
        return impl->input && k != Key::Count && impl->input->isDown(k);
    };
    inputTbl["was_pressed"] = [this](const std::string& name) {
        Key k = keyFromName(name);
        return impl->input && k != Key::Count && impl->input->wasPressed(k);
    };

    impl->initialized = true;
    return true;
}

void ScriptHost::shutdown() {
    if (!impl) return;
    impl->panels.clear();
    impl->timers.clear();
    impl->files.clear();
    impl->initialized = false;
}

sol::state& ScriptHost::lua() { return impl->lua; }

void ScriptHost::setScriptRoot(std::string dir) {
    impl->root = std::move(dir);
    // Let plain require() find script files too.
    impl->lua["package"]["path"] = impl->root + "/?.lua;" +
                                   impl->lua["package"]["path"].get<std::string>();
}

bool ScriptHost::runFile(const std::string& relPath) {
    for (auto& file : impl->files) {
        if (file.rel == relPath) return impl->run(file); // re-run, already tracked
    }

    Impl::TrackedFile file;
    file.rel = relPath;
    file.resolved = impl->resolve(relPath);
    std::error_code ec;
    file.mtime = fs::last_write_time(file.resolved, ec);
    if (ec) {
        std::cerr << "[typengine/lua] script not found: " << relPath << "\n";
        return false;
    }
    impl->files.push_back(std::move(file));
    return impl->run(impl->files.back());
}

void ScriptHost::update(const Input& input, float dt) {
    if (!impl->initialized) return;
    impl->input = &input;

    // Timers: collect due ones first — callbacks may schedule new timers.
    std::vector<sol::protected_function> due;
    for (auto it = impl->timers.begin(); it != impl->timers.end();) {
        it->remaining -= dt;
        if (it->remaining <= 0.0f) {
            due.push_back(std::move(it->fn));
            it = impl->timers.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& fn : due) {
        sol::protected_function_result r = fn();
        if (!r.valid()) logScriptError("timer", r);
    }

    // Hot-reload poll (~1s).
    impl->reloadTimer += dt;
    if (impl->reloadTimer >= 1.0f) {
        impl->reloadTimer = 0.0f;
        for (auto& file : impl->files) {
            std::error_code ec;
            auto mtime = fs::last_write_time(file.resolved, ec);
            if (ec || mtime == file.mtime) continue;
            file.mtime = mtime;
            std::cout << "[typengine/lua] reloading " << file.rel << std::endl;
            if (impl->run(file)) {
                emit("script_reloaded", {{"file", file.rel}});
            }
        }
    }

    for (auto& [name, panel] : impl->panels) panel->update(input);
}

void ScriptHost::render(Renderer& renderer) {
    if (!impl->initialized) return;
    for (auto& [name, panel] : impl->panels) panel->render(renderer);
}

void ScriptHost::emit(const std::string& event,
                      std::initializer_list<std::pair<const char*, ScriptValue>> fields) {
    if (!impl->initialized) return;
    sol::table payload = impl->lua.create_table();
    for (const auto& [key, value] : fields) {
        std::visit([&](const auto& v) { payload[key] = v; }, value);
    }
    impl->dispatch(event, payload);
}

void ScriptHost::emitTable(const std::string& event, sol::table payload) {
    if (!impl->initialized) return;
    impl->dispatch(event, payload);
}

} // namespace ty
