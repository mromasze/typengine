#pragma once
// Typengine — Renderer.hpp
// SDL2-backed 2D renderer with a layer-sorted draw queue.
// Public header is SDL-free; all SDL usage lives in Renderer.cpp.

#include <memory>

#include "engine/Math.hpp"

namespace ty {

using TextureId = int;
inline constexpr TextureId InvalidTexture = -1;

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Called by Engine::initialize(); `sdlWindow` is an SDL_Window*.
    bool initialize(void* sdlWindow, bool vsync);
    void shutdown();

    void beginFrame();
    // Flushes the draw queue (stable-sorted by layer) and presents.
    void endFrame();

    void setClearColor(Color c);
    void setVSync(bool enabled);

    // Loads PNG/JPG/BMP... via stb_image. Returns InvalidTexture on failure.
    TextureId loadTexture(const char* path);
    // 1x1 texture of a solid color — handy for bars, tints and placeholders.
    TextureId createSolidTexture(Color c);
    // Texture from raw RGBA8 pixels (w*h*4 bytes) — procedural content.
    TextureId createTexture(const void* rgbaPixels, int w, int h);
    Vec2i textureSize(TextureId id) const;

    // Queued draws; `layer` decides ordering (lower = drawn first).
    void drawTexture(TextureId id, const Rect& src, const Rect& dst, int layer = 0);
    void drawFilledRect(const Rect& dst, Color c, int layer = 0);
    // Debug seven-segment number rendering (no font asset needed).
    void drawNumber(int value, Vec2 pos, float size, Color c, int layer = 100);

    int width() const;
    int height() const;
    void setLogicalResolution(int w, int h);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace ty
