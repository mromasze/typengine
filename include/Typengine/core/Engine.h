#pragma once
#include <string>

namespace Typengine::Core {

class Engine {
public:
    static Engine& Get() { static Engine instance; return instance; }

    bool Initialize(const char* title, int width, int height);
    void Cleanup();
    bool PollEvents();
    void Update(); 

    float GetTime();
    
    bool IsVSyncEnabled() const { return vsync; }
    int GetFPSLimit() const { return fpsLimit; }

    int GetScreenWidth() const;
    int GetScreenHeight() const;

    void SetPaused(bool p) { isPaused = p; }
    bool IsPaused() const { return isPaused; }

private:
    Engine() = default;
    void LoadConfig();
    
    std::string configPath;
    long long lastConfigTime = 0;
    float nextConfigCheck = 0.0f;

    bool vsync = true;
    int fpsLimit = 60;
    bool isPaused = false;
};

} 
