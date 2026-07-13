#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "engine/Renderer.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace ty {

namespace {

struct TextureResource {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
};

struct DrawCmd {
    enum class Type { Texture, FilledRect } type = Type::Texture;
    TextureId textureId = InvalidTexture;
    Rect src{};
    Rect dst{};
    Color color{};
    int layer = 0;
};

} // namespace

struct Renderer::Impl {
    SDL_Renderer* sdl = nullptr;
    std::vector<TextureResource> textures;
    std::vector<DrawCmd> queue;
    Color clearColor{15, 15, 20, 255};
    int width = 1280;
    int height = 720;
};

Renderer::Renderer() : impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() { shutdown(); }

bool Renderer::initialize(void* sdlWindow, bool vsync) {
    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

    impl->sdl = SDL_CreateRenderer(static_cast<SDL_Window*>(sdlWindow), -1, flags);
    if (!impl->sdl) {
        std::cerr << "[typengine] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GetWindowSize(static_cast<SDL_Window*>(sdlWindow), &impl->width, &impl->height);
    SDL_RenderSetLogicalSize(impl->sdl, impl->width, impl->height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest-neighbor for pixel art
    return true;
}

void Renderer::shutdown() {
    if (!impl) return;
    for (auto& t : impl->textures) {
        if (t.texture) SDL_DestroyTexture(t.texture);
    }
    impl->textures.clear();
    if (impl->sdl) {
        SDL_DestroyRenderer(impl->sdl);
        impl->sdl = nullptr;
    }
}

void Renderer::beginFrame() {
    impl->queue.clear();
    SDL_SetRenderDrawColor(impl->sdl, impl->clearColor.r, impl->clearColor.g,
                           impl->clearColor.b, impl->clearColor.a);
    SDL_RenderClear(impl->sdl);
}

void Renderer::endFrame() {
    auto& queue = impl->queue;
    std::stable_sort(queue.begin(), queue.end(),
                     [](const DrawCmd& a, const DrawCmd& b) { return a.layer < b.layer; });

    for (const auto& cmd : queue) {
        if (cmd.type == DrawCmd::Type::FilledRect) {
            SDL_SetRenderDrawBlendMode(impl->sdl, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(impl->sdl, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
            SDL_FRect d = {cmd.dst.x, cmd.dst.y, cmd.dst.w, cmd.dst.h};
            SDL_RenderFillRectF(impl->sdl, &d);
        } else if (cmd.textureId >= 0 &&
                   static_cast<std::size_t>(cmd.textureId) < impl->textures.size()) {
            SDL_Rect s = {static_cast<int>(cmd.src.x), static_cast<int>(cmd.src.y),
                          static_cast<int>(cmd.src.w), static_cast<int>(cmd.src.h)};
            SDL_FRect d = {cmd.dst.x, cmd.dst.y, cmd.dst.w, cmd.dst.h};
            SDL_RenderCopyF(impl->sdl, impl->textures[cmd.textureId].texture, &s, &d);
        }
    }
    SDL_RenderPresent(impl->sdl);
}

void Renderer::setClearColor(Color c) { impl->clearColor = c; }

void Renderer::setVSync(bool enabled) {
    if (!impl->sdl) return;
    if (SDL_RenderSetVSync(impl->sdl, enabled ? 1 : 0) != 0) {
        std::cerr << "[typengine] SDL_RenderSetVSync unsupported: " << SDL_GetError() << "\n";
    }
}

TextureId Renderer::loadTexture(const char* path) {
    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) {
        std::string fallback = std::string("../") + path;
        pixels = stbi_load(fallback.c_str(), &w, &h, &channels, 4);
        if (!pixels) {
            std::cerr << "[typengine] Failed to load texture: " << path << "\n";
            return InvalidTexture;
        }
    }

    SDL_Texture* tex = SDL_CreateTexture(impl->sdl, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex) {
        stbi_image_free(pixels);
        return InvalidTexture;
    }
    SDL_UpdateTexture(tex, nullptr, pixels, w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    stbi_image_free(pixels);

    impl->textures.push_back({tex, w, h});
    return static_cast<TextureId>(impl->textures.size()) - 1;
}

TextureId Renderer::createSolidTexture(Color c) {
    const Uint8 pixel[4] = {c.r, c.g, c.b, c.a};
    return createTexture(pixel, 1, 1);
}

TextureId Renderer::createTexture(const void* rgbaPixels, int w, int h) {
    SDL_Texture* tex = SDL_CreateTexture(impl->sdl, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex) return InvalidTexture;
    SDL_UpdateTexture(tex, nullptr, rgbaPixels, w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    impl->textures.push_back({tex, w, h});
    return static_cast<TextureId>(impl->textures.size()) - 1;
}

Vec2i Renderer::textureSize(TextureId id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= impl->textures.size()) return {0, 0};
    return {impl->textures[id].width, impl->textures[id].height};
}

void Renderer::drawTexture(TextureId id, const Rect& src, const Rect& dst, int layer) {
    impl->queue.push_back({DrawCmd::Type::Texture, id, src, dst, {}, layer});
}

void Renderer::drawFilledRect(const Rect& dst, Color c, int layer) {
    impl->queue.push_back({DrawCmd::Type::FilledRect, InvalidTexture, {}, dst, c, layer});
}

void Renderer::drawNumber(int value, Vec2 pos, float size, Color c, int layer) {
    static constexpr int segments[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 1},
        {1, 1, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 0, 1, 1},
        {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 1, 1}};

    std::string str = std::to_string(value);
    const float s = size;
    const float s2 = s / 2.0f;
    const float t = std::max(1.0f, s / 5.0f);
    float cursorX = pos.x;

    for (char ch : str) {
        if (ch == '-') {
            drawFilledRect({cursorX, pos.y + s2 - t / 2.0f, s, t}, c, layer);
            cursorX += s * 1.2f;
            continue;
        }
        if (ch < '0' || ch > '9') continue;
        const int* seg = segments[ch - '0'];
        if (seg[0]) drawFilledRect({cursorX, pos.y, s, t}, c, layer);
        if (seg[1]) drawFilledRect({cursorX + s - t, pos.y, t, s2}, c, layer);
        if (seg[2]) drawFilledRect({cursorX + s - t, pos.y + s2, t, s2}, c, layer);
        if (seg[3]) drawFilledRect({cursorX, pos.y + s - t, s, t}, c, layer);
        if (seg[4]) drawFilledRect({cursorX, pos.y + s2, t, s2}, c, layer);
        if (seg[5]) drawFilledRect({cursorX, pos.y, t, s2}, c, layer);
        if (seg[6]) drawFilledRect({cursorX, pos.y + s2 - t / 2.0f, s, t}, c, layer);
        cursorX += s * 1.2f;
    }
}

int Renderer::width() const { return impl->width; }
int Renderer::height() const { return impl->height; }

void Renderer::setLogicalResolution(int w, int h) {
    impl->width = w;
    impl->height = h;
    if (impl->sdl) SDL_RenderSetLogicalSize(impl->sdl, w, h);
}

} // namespace ty
