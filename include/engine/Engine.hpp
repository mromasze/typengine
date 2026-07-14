#pragma once
// Typengine — Engine.hpp
// Window + main loop + subsystem ownership + scene stack.
//
// Minimal usage:
//   ty::Engine engine;
//   engine.initialize({.title = "My Game", .width = 1280, .height = 720});
//   engine.run(std::make_unique<MyScene>());
//
// Or drive the loop yourself with pollEvents()/beginFrame()/endFrame().

#include <memory>
#include <string>

#include "engine/Input.hpp"
#include "engine/JobSystem.hpp"
#include "engine/Renderer.hpp"

struct SDL_Window;

namespace ty {

class Engine;

// A game state (menu, dungeon, hub...). The engine calls the hooks;
// switch scenes with Engine::setScene() — the swap happens between frames.
class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter(Engine&) {}
    virtual void onExit(Engine&) {}
    virtual void update(Engine& engine, float dt) = 0;
    virtual void render(Engine& engine) = 0;
};

struct EngineConfig {
    std::string title = "Typengine";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    int fpsLimit = 60;              // used when vsync is off; 0 = uncapped
    bool showFps = false;           // FPS overlay, top-right corner
    // Worker threads for jobs()/parallelFor. 0 = auto (hardware threads - 1).
    unsigned workerThreads = 0;
    // Optional key=value config file, hot-reloaded at runtime
    // (supported keys: vsync, fps_limit, show_fps). Empty = disabled.
    std::string configFile{};
};

class Engine {
public:
    Engine();
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool initialize(const EngineConfig& config);
    void shutdown();

    Renderer& renderer() { return renderer_; }
    Input& input() { return input_; }
    JobSystem& jobs() { return jobs_; }

    // --- scene-driven loop -------------------------------------------------
    // Runs until quit() or the window is closed. Takes ownership of `first`.
    void run(std::unique_ptr<Scene> first);
    // Deferred scene switch (safe to call from inside update()).
    void setScene(std::unique_ptr<Scene> next);
    void quit() { running_ = false; }

    // --- manual loop ---------------------------------------------------------
    bool pollEvents();              // false when the window was closed
    float beginFrame();             // input update, config reload, returns dt
    void endFrame();                // fps limiting when vsync is off

    float time() const;             // seconds since start
    int screenWidth() const;
    int screenHeight() const;

    void setPaused(bool p) { paused_ = p; }
    bool isPaused() const { return paused_; }

    // Measured frames per second (updated twice a second).
    float fps() const { return fps_; }
    void setShowFps(bool show) { config_.showFps = show; }
    bool isShowFpsEnabled() const { return config_.showFps; }

private:
    void loadConfigFile();

    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    Input input_;
    JobSystem jobs_;

    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Scene> pendingScene_;

    EngineConfig config_;
    bool running_ = false;
    bool paused_ = false;
    bool initialized_ = false;

    float lastFrameTime_ = 0.0f;
    float frameStartTime_ = 0.0f;

    float fps_ = 0.0f;
    int fpsFrames_ = 0;
    float fpsAccum_ = 0.0f;

    long long configMTime_ = 0;
    float nextConfigCheck_ = 0.0f;
};

} // namespace ty
