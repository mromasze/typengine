#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "engine/Renderer.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
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
    TextureId fontTexture = InvalidTexture; // lazy-built 5x7 ASCII strip
    std::unordered_map<std::string, TextureId> pathCache; // dedup loads across scenes
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
    impl->pathCache.clear();
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
            SDL_Texture* tex = impl->textures[cmd.textureId].texture;
            // Tint defaults to opaque white (a no-op modulation).
            SDL_SetTextureColorMod(tex, cmd.color.r, cmd.color.g, cmd.color.b);
            SDL_SetTextureAlphaMod(tex, cmd.color.a);
            SDL_Rect s = {static_cast<int>(cmd.src.x), static_cast<int>(cmd.src.y),
                          static_cast<int>(cmd.src.w), static_cast<int>(cmd.src.h)};
            SDL_FRect d = {cmd.dst.x, cmd.dst.y, cmd.dst.w, cmd.dst.h};
            SDL_RenderCopyF(impl->sdl, tex, &s, &d);
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
    if (auto it = impl->pathCache.find(path); it != impl->pathCache.end()) return it->second;

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
    TextureId id = static_cast<TextureId>(impl->textures.size()) - 1;
    impl->pathCache.emplace(path, id);
    return id;
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

namespace {

// Classic 5x7 pixel font, ASCII 32-126. 5 bytes per glyph, one byte per
// column, bit 0 = top row (the widely-used "glcdfont" layout).
constexpr unsigned char kFont5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x04, 0x08, 0x10, 0x08}, // ~
};

constexpr int GlyphW = 5;
constexpr int GlyphH = 7;
constexpr int GlyphAdvance = 6; // 5px glyph + 1px spacing
constexpr int GlyphCount = 95;

} // namespace

void Renderer::drawText(const char* text, Vec2 pos, float scale, Color c, int layer) {
    if (!text) return;

    // Build the font strip once: 95 glyphs side by side, white on transparent,
    // 6px cells (5px glyph + 1px gap) so src rects can use the full cell.
    if (impl->fontTexture == InvalidTexture) {
        const int w = GlyphAdvance * GlyphCount;
        std::vector<unsigned char> pixels(static_cast<std::size_t>(w) * GlyphH * 4, 0);
        for (int g = 0; g < GlyphCount; ++g) {
            for (int col = 0; col < GlyphW; ++col) {
                unsigned char bits = kFont5x7[g][col];
                for (int row = 0; row < GlyphH; ++row) {
                    if (bits & (1u << row)) {
                        std::size_t idx =
                            (static_cast<std::size_t>(row) * w + g * GlyphAdvance + col) * 4;
                        pixels[idx + 0] = 255;
                        pixels[idx + 1] = 255;
                        pixels[idx + 2] = 255;
                        pixels[idx + 3] = 255;
                    }
                }
            }
        }
        impl->fontTexture = createTexture(pixels.data(), w, GlyphH);
        if (impl->fontTexture == InvalidTexture) return;
    }

    float x = pos.x;
    float y = pos.y;
    for (const char* p = text; *p; ++p) {
        char ch = *p;
        if (ch == '\n') {
            x = pos.x;
            y += (GlyphH + 2) * scale;
            continue;
        }
        if (ch >= 32 && ch < 32 + GlyphCount && ch != ' ') {
            Rect src = {static_cast<float>((ch - 32) * GlyphAdvance), 0.0f,
                        static_cast<float>(GlyphW), static_cast<float>(GlyphH)};
            Rect dst = {x, y, GlyphW * scale, GlyphH * scale};
            impl->queue.push_back({DrawCmd::Type::Texture, impl->fontTexture, src, dst, c, layer});
        }
        x += GlyphAdvance * scale;
    }
}

Vec2 Renderer::measureText(const char* text, float scale) const {
    if (!text) return {0, 0};
    float maxW = 0;
    float lines = 1;
    float w = 0;
    for (const char* p = text; *p; ++p) {
        if (*p == '\n') {
            maxW = std::max(maxW, w);
            w = 0;
            lines += 1;
            continue;
        }
        w += GlyphAdvance * scale;
    }
    maxW = std::max(maxW, w);
    if (maxW > 0) maxW -= scale; // no trailing 1px gap after the last glyph
    return {maxW, lines * GlyphH * scale + (lines - 1) * 2 * scale};
}

int Renderer::width() const { return impl->width; }
int Renderer::height() const { return impl->height; }

void Renderer::setLogicalResolution(int w, int h) {
    impl->width = w;
    impl->height = h;
    if (impl->sdl) SDL_RenderSetLogicalSize(impl->sdl, w, h);
}

} // namespace ty
