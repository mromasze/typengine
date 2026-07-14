#pragma once
// Typengine — IsoTilemap.hpp
// Isometric tilemap: flat TileGrid + tileset atlas + culled diamond rendering.
// Tile id 0 is "empty" (never drawn). Walkability is a per-tile-id property.

#include <cstdint>
#include <vector>

#include "engine/IsoCamera.hpp"
#include "engine/Math.hpp"
#include "engine/Renderer.hpp"
#include "engine/Tilemap.hpp"

namespace ty {

// Atlas layout: tiles packed left-to-right, top-to-bottom, `columns` per row.
// tileId N (N >= 1) reads cell N-1 of the atlas.
struct Tileset {
    TextureId texture = InvalidTexture;
    int tileW = 64;      // source cell width in the atlas
    int tileH = 32;      // source cell height (may exceed the grid diamond, e.g. walls)
    int columns = 1;
};

class IsoTilemap {
public:
    IsoTilemap() = default;
    IsoTilemap(int w, int h, const Tileset& ts) : tileset_(ts) { grid_.resize(w, h); }

    void resize(int w, int h, TileId fill = 0) { grid_.resize(w, h, fill); }
    void setTileset(const Tileset& ts) { tileset_ = ts; }
    const Tileset& tileset() const { return tileset_; }

    TileGrid& grid() { return grid_; }
    const TileGrid& grid() const { return grid_; }

    TileId at(int x, int y) const { return grid_.at(x, y); }
    void set(int x, int y, TileId v) { grid_.set(x, y, v); }
    int width() const { return grid_.width(); }
    int height() const { return grid_.height(); }

    // --- walkability -----------------------------------------------------
    // Marks a tile *id* as walkable/blocked (default: everything blocked,
    // including empty tile 0).
    void setWalkable(TileId id, bool walkable) {
        if (id >= walkable_.size()) walkable_.resize(id + 1, 0);
        walkable_[id] = walkable ? 1 : 0;
    }
    bool isTileWalkable(TileId id) const {
        return id < walkable_.size() && walkable_[id] != 0;
    }
    // World point (tile units) on a walkable tile?
    bool isWalkable(Vec2 world) const {
        int tx = static_cast<int>(std::floor(world.x));
        int tyy = static_cast<int>(std::floor(world.y));
        if (!grid_.inBounds(tx, tyy)) return false;
        return isTileWalkable(grid_.at(tx, tyy));
    }
    // AABB (world/tile units) fully on walkable ground? Checks all four corners.
    bool isAreaWalkable(const Rect& aabb) const {
        return isWalkable({aabb.x, aabb.y}) &&
               isWalkable({aabb.x + aabb.w, aabb.y}) &&
               isWalkable({aabb.x, aabb.y + aabb.h}) &&
               isWalkable({aabb.x + aabb.w, aabb.y + aabb.h});
    }

    // --- lighting / fog of war ----------------------------------------------
    // Optional per-tile brightness (0 = pitch black, tile isn't drawn at all;
    // 255 = fully lit). Game code writes it every frame from its visibility
    // system; disabled maps render fully lit.
    void enableLighting(std::uint8_t initial = 0) {
        light_.assign(grid_.size(), initial);
    }
    void disableLighting() { light_.clear(); }
    bool lightingEnabled() const { return light_.size() == grid_.size() && !light_.empty(); }
    void setLight(int x, int y, std::uint8_t v) {
        if (lightingEnabled() && grid_.inBounds(x, y))
            light_[static_cast<std::size_t>(y) * grid_.width() + x] = v;
    }
    std::uint8_t lightAt(int x, int y) const {
        if (!lightingEnabled()) return 255;
        if (!grid_.inBounds(x, y)) return 0;
        return light_[static_cast<std::size_t>(y) * grid_.width() + x];
    }
    // World-space sample (tile units) — for tinting entities by the light of
    // the tile they stand on.
    std::uint8_t lightAtWorld(Vec2 world) const {
        return lightAt(static_cast<int>(std::floor(world.x)),
                       static_cast<int>(std::floor(world.y)));
    }

    // --- rendering ---------------------------------------------------------
    // Draws all tiles visible through `camera`, culled to the viewport.
    // `layer` is the renderer layer for the whole map (entities go above).
    void render(Renderer& renderer, const IsoCamera& camera, int layer = 0) const;

private:
    TileGrid grid_;
    Tileset tileset_;
    std::vector<std::uint8_t> walkable_;
    std::vector<std::uint8_t> light_;   // empty = lighting disabled
};

} // namespace ty
