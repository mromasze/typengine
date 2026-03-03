#pragma once
#include <vector>
#include "Typengine/core/Math.h"

namespace Typengine::Renderer {

struct Rect {
    float x, y, w, h;
};

class Renderer {
public:
    static Renderer& Get() { static Renderer instance; return instance; }

    bool Initialize(void* window, bool vsync);
    void Cleanup();

    void BeginFrame();
    void EndFrame();

    void SetVSync(bool enabled);

    int LoadTexture(const char* path);
    void DrawTexture(int textureId, const Rect& src, const Rect& dst, int layer = 0);

    int GetWidth() const;
    int GetHeight() const;
    void SetResolution(int w, int h);

private:
    Renderer() = default;
};

} 
