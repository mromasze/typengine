#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Typengine/renderer/Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <algorithm>

namespace Typengine::Renderer {

struct TextureResource {
    SDL_Texture* texture;
    int width, height;
};

struct DrawCmd {
    int textureId;
    Rect src, dst;
    int layer;
};

static SDL_Renderer* sdlRenderer = nullptr;
static std::vector<TextureResource> textures;
static std::vector<DrawCmd> drawQueue;
static int renderWidth = 800;
static int renderHeight = 600;

bool Renderer::Initialize(void* window, bool vsync) {
    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

    sdlRenderer = SDL_CreateRenderer((SDL_Window*)window, -1, flags);
    if (!sdlRenderer) return false;

    SDL_RenderSetLogicalSize(sdlRenderer, renderWidth, renderHeight);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    return true;
}

void Renderer::Cleanup() {
    for (auto& t : textures) if (t.texture) SDL_DestroyTexture(t.texture);
    textures.clear();
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
}

void Renderer::BeginFrame() {
    drawQueue.clear();
    SDL_SetRenderDrawColor(sdlRenderer, 25, 100, 200, 255);
    SDL_RenderClear(sdlRenderer);
}

void Renderer::EndFrame() {
    std::stable_sort(drawQueue.begin(), drawQueue.end(), [](const DrawCmd& a, const DrawCmd& b) {
        return a.layer < b.layer;
    });

    for (const auto& cmd : drawQueue) {
        if (cmd.textureId >= 0 && (size_t)cmd.textureId < textures.size()) {
            SDL_Rect s = {(int)cmd.src.x, (int)cmd.src.y, (int)cmd.src.w, (int)cmd.src.h};
            SDL_Rect d = {(int)cmd.dst.x, (int)cmd.dst.y, (int)cmd.dst.w, (int)cmd.dst.h};
            SDL_RenderCopy(sdlRenderer, textures[cmd.textureId].texture, &s, &d);
        }
    }
    SDL_RenderPresent(sdlRenderer);
}

void Renderer::SetVSync(bool enabled) {
    if (sdlRenderer) {
        
        
        
        int result = SDL_RenderSetVSync(sdlRenderer, enabled ? 1 : 0);
        if (result != 0) {
            std::cerr << "[Typengine] Warning: SDL_RenderSetVSync failed or not supported: " << SDL_GetError() << "\n";
        }
    }
}

int Renderer::LoadTexture(const char* path) {
    int w, h, ch;
    stbi_uc* pixels = stbi_load(path, &w, &h, &ch, 4);
    if (!pixels) {
        std::string fallback = std::string("../") + path;
        pixels = stbi_load(fallback.c_str(), &w, &h, &ch, 4);
        if (!pixels) return -1;
    }

    SDL_Texture* tex = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    SDL_UpdateTexture(tex, NULL, pixels, w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    stbi_image_free(pixels);

    textures.push_back({tex, w, h});
    return (int)textures.size() - 1;
}

void Renderer::DrawTexture(int textureId, const Rect& src, const Rect& dst, int layer) {
    drawQueue.push_back({textureId, src, dst, layer});
}

int Renderer::GetWidth() const { return renderWidth; }
int Renderer::GetHeight() const { return renderHeight; }
void Renderer::SetResolution(int w, int h) {
    renderWidth = w;
    renderHeight = h;
    if (sdlRenderer) SDL_RenderSetLogicalSize(sdlRenderer, w, h);
}

} 
