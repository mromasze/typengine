#include "engine/Engine.hpp"

#include <SDL2/SDL.h>

#include <fstream>
#include <iostream>
#include <sys/stat.h>

namespace ty {

namespace {

long long fileWriteTime(const std::string& path) {
    struct stat s{};
    if (stat(path.c_str(), &s) == 0) return static_cast<long long>(s.st_mtime);
    return 0;
}

std::string trim(const std::string& s) {
    std::size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    std::size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

} // namespace

Engine::Engine() = default;
Engine::~Engine() { shutdown(); }

bool Engine::initialize(const EngineConfig& config) {
    config_ = config;
    loadConfigFile();

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[typengine] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    window_ = SDL_CreateWindow(config_.title.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, config_.width, config_.height,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        std::cerr << "[typengine] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    if (!renderer_.initialize(window_, config_.vsync)) return false;

    jobs_.initialize(config_.workerThreads);

    initialized_ = true;
    lastFrameTime_ = time();
    return true;
}

void Engine::shutdown() {
    if (!initialized_) return;
    if (scene_) {
        scene_->onExit(*this);
        scene_.reset();
    }
    pendingScene_.reset();
    jobs_.shutdown();
    renderer_.shutdown();
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
    initialized_ = false;
}

void Engine::run(std::unique_ptr<Scene> first) {
    scene_ = std::move(first);
    if (scene_) scene_->onEnter(*this);

    running_ = true;
    while (running_ && pollEvents()) {
        float dt = beginFrame();

        if (pendingScene_) {
            if (scene_) scene_->onExit(*this);
            scene_ = std::move(pendingScene_);
            scene_->onEnter(*this);
        }

        if (scene_) {
            scene_->update(*this, dt);
            renderer_.beginFrame();
            scene_->render(*this);
            if (config_.showFps) {
                std::string text = "FPS " + std::to_string(static_cast<int>(fps_ + 0.5f));
                float w = renderer_.measureText(text.c_str(), 2.0f).x;
                renderer_.drawText(text.c_str(),
                                   {static_cast<float>(renderer_.width()) - w - 12.0f, 8.0f},
                                   2.0f, {120, 255, 140, 255}, 100000);
            }
            renderer_.endFrame();
        }

        endFrame();
    }
    running_ = false;
}

void Engine::setScene(std::unique_ptr<Scene> next) { pendingScene_ = std::move(next); }

bool Engine::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}

float Engine::beginFrame() {
    frameStartTime_ = time();
    float dt = frameStartTime_ - lastFrameTime_;
    lastFrameTime_ = frameStartTime_;

    ++fpsFrames_;
    fpsAccum_ += dt;
    if (fpsAccum_ >= 0.5f) {
        fps_ = fpsFrames_ / fpsAccum_;
        fpsFrames_ = 0;
        fpsAccum_ = 0.0f;
    }

    if (dt > 0.25f) dt = 0.25f; // clamp huge hitches (debugger pauses, window drags)

    input_.update();

    if (!config_.configFile.empty() && frameStartTime_ > nextConfigCheck_) {
        loadConfigFile();
        nextConfigCheck_ = frameStartTime_ + 1.0f;
    }
    return dt;
}

void Engine::endFrame() {
    if (config_.vsync || config_.fpsLimit <= 0) return;
    float targetDt = 1.0f / static_cast<float>(config_.fpsLimit);
    float elapsed = time() - frameStartTime_;
    if (elapsed < targetDt) {
        SDL_Delay(static_cast<Uint32>((targetDt - elapsed) * 1000.0f));
    }
}

float Engine::time() const { return static_cast<float>(SDL_GetTicks64()) / 1000.0f; }

void Engine::setFullscreen(bool fs) {
    if (!window_ || fs == fullscreen_) return;
    SDL_SetWindowFullscreen(window_, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    fullscreen_ = fs;
}

void Engine::setWindowSize(int w, int h) {
    if (!window_ || w <= 0 || h <= 0) return;
    config_.width = w;
    config_.height = h;
    if (!fullscreen_) {
        SDL_SetWindowSize(window_, w, h);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    renderer_.setLogicalResolution(w, h);
}

void Engine::setVSync(bool enabled) {
    config_.vsync = enabled;
    renderer_.setVSync(enabled);
}

int Engine::screenWidth() const { return renderer_.width(); }
int Engine::screenHeight() const { return renderer_.height(); }

void Engine::loadConfigFile() {
    if (config_.configFile.empty()) return;

    long long mtime = fileWriteTime(config_.configFile);
    if (mtime == 0 || (mtime <= configMTime_ && configMTime_ != 0)) return;
    configMTime_ = mtime;

    std::ifstream f(config_.configFile);
    if (!f.is_open()) return;

    bool oldVsync = config_.vsync;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::size_t sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = trim(line.substr(0, sep));
        std::string val = trim(line.substr(sep + 1));

        if (key == "vsync") {
            config_.vsync = (val == "true" || val == "1");
        } else if (key == "fps_limit") {
            try {
                config_.fpsLimit = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "show_fps") {
            config_.showFps = (val == "true" || val == "1");
        }
    }

    if (initialized_ && config_.vsync != oldVsync) {
        renderer_.setVSync(config_.vsync);
        std::cout << "[typengine] vsync: " << (config_.vsync ? "on" : "off") << "\n";
    }
}

} // namespace ty
