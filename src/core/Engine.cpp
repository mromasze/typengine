#include "Typengine/core/Engine.h"
#include "Typengine/renderer/Renderer.h"
#include "Typengine/input/Input.h"
#include "Typengine/ui/UISystem.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

namespace Typengine::Core {

static SDL_Window* window = nullptr;

static long long GetFileWriteTime(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) return (long long)s.st_mtime;
    return 0;
}

static std::string Trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return s;
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

void Engine::LoadConfig() {
    if (configPath.empty()) {
        std::ifstream f1("engine_config.txt");
        if (f1.is_open()) configPath = "engine_config.txt";
        else {
            std::ifstream f2("../engine_config.txt");
            if (f2.is_open()) configPath = "../engine_config.txt";
            else return;
        }
    }

    long long mtime = GetFileWriteTime(configPath);
    if (mtime <= lastConfigTime && lastConfigTime != 0) return;
    lastConfigTime = mtime;

    std::ifstream f(configPath);
    if (!f.is_open()) return;

    std::cout << "[Typengine] Config change detected. Reloading " << configPath << "...\n";
    
    bool oldVsync = vsync;
    std::string line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = Trim(line.substr(0, sep));
        std::string val = Trim(line.substr(sep + 1));

        if (key == "vsync") {
            vsync = (val == "true" || val == "1");
        } else if (key == "fps_limit") {
            fpsLimit = std::stoi(val);
        }
    }

    if (vsync != oldVsync) {
        Renderer::Renderer::Get().SetVSync(vsync);
        std::cout << "[Typengine] VSync: " << (vsync ? "Enabled" : "Disabled") << "\n";
    }
    std::cout << "[Typengine] FPS Limit: " << fpsLimit << "\n";
}

bool Engine::Initialize(const char* title, int width, int height) {
    LoadConfig();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) return false;

    if (!Renderer::Renderer::Get().Initialize(window, vsync)) return false;

    return true;
}

void Engine::Update() {
    Input::Input::Get().Update();
    UI::UISystem::Get().Update();

    
    float time = GetTime();
    if (time > nextConfigCheck) {
        LoadConfig();
        nextConfigCheck = time + 1.0f;
    }
}

void Engine::Cleanup() {
    Renderer::Renderer::Get().Cleanup();
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Engine::PollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}

float Engine::GetTime() {
    return SDL_GetTicks() / 1000.0f;
}

int Engine::GetScreenWidth() const { return Renderer::Renderer::Get().GetWidth(); }
int Engine::GetScreenHeight() const { return Renderer::Renderer::Get().GetHeight(); }

} 
