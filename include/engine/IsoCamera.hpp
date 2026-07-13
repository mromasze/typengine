#pragma once
// Typengine — IsoCamera.hpp
//
// Coordinate convention used across the engine:
//   * Gameplay ("world") space is CARTESIAN, measured in TILE UNITS.
//     Entity at world (3.5, 2.0) stands in the middle of tile column 3,
//     row 2. Movement and collision are plain axis-aligned math there.
//   * The camera projects world space onto the screen with the classic
//     2:1 diamond isometric projection:
//         screen.x = (world.x - world.y) * tileWidth  / 2
//         screen.y = (world.x + world.y) * tileHeight / 2
//   * Depth sorting for entities: bigger (world.x + world.y) is "closer".

#include "engine/Math.hpp"

namespace ty {

class IsoCamera {
public:
    IsoCamera() = default;
    IsoCamera(int tileWidth, int tileHeight) : tileW(tileWidth), tileH(tileHeight) {}

    void setTileSize(int tileWidth, int tileHeight) { tileW = tileWidth; tileH = tileHeight; }
    void setViewport(int w, int h) { viewportW = w; viewportH = h; }
    void setZoom(float z) { zoom = z < 0.1f ? 0.1f : z; }
    float getZoom() const { return zoom; }

    // Camera position in world (tile) units — the point kept at screen center.
    void setPosition(Vec2 worldPos) { position = worldPos; }
    Vec2 getPosition() const { return position; }

    // Smoothly track a target (frame-rate independent).
    void follow(Vec2 target, float speed, float dt) {
        position = Vec2::lerp(position, target, smoothFactor(speed, dt));
    }

    // Isometric projection of a world point (before camera offset).
    Vec2 project(Vec2 world) const {
        return {(world.x - world.y) * (tileW * 0.5f),
                (world.x + world.y) * (tileH * 0.5f)};
    }
    Vec2 unproject(Vec2 iso) const {
        float ix = iso.x / (tileW * 0.5f);
        float iy = iso.y / (tileH * 0.5f);
        return {(ix + iy) * 0.5f, (iy - ix) * 0.5f};
    }

    Vec2 worldToScreen(Vec2 world) const {
        Vec2 iso = project(world);
        Vec2 camIso = project(position);
        return {(iso.x - camIso.x) * zoom + viewportW * 0.5f,
                (iso.y - camIso.y) * zoom + viewportH * 0.5f};
    }

    Vec2 screenToWorld(Vec2 screen) const {
        Vec2 camIso = project(position);
        Vec2 iso = {(screen.x - viewportW * 0.5f) / zoom + camIso.x,
                    (screen.y - viewportH * 0.5f) / zoom + camIso.y};
        return unproject(iso);
    }

    // Tile the given world point falls into.
    Vec2i worldToTile(Vec2 world) const {
        return {static_cast<int>(std::floor(world.x)), static_cast<int>(std::floor(world.y))};
    }
    // World-space center of a tile.
    Vec2 tileToWorld(int tx, int tyy) const {
        return {static_cast<float>(tx) + 0.5f, static_cast<float>(tyy) + 0.5f};
    }

    int tileWidth() const { return tileW; }
    int tileHeight() const { return tileH; }

private:
    Vec2 position{};
    float zoom = 1.0f;
    int tileW = 64;
    int tileH = 32;
    int viewportW = 800;
    int viewportH = 600;
};

// Depth key for isometric painter's ordering of entities.
inline float isoDepth(Vec2 world) { return world.x + world.y; }

} // namespace ty
