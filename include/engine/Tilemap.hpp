#pragma once
// Typengine — Tilemap.hpp
// Flat, contiguous tile storage (replaces the old vector<vector<int>> map).
// Row-major: index = y * width + x.

#include <cstdint>
#include <vector>

namespace ty {

using TileId = std::uint16_t;

class TileGrid {
public:
    TileGrid() = default;
    TileGrid(int w, int h, TileId fill = 0) { resize(w, h, fill); }

    void resize(int w, int h, TileId fill = 0) {
        width_ = w;
        height_ = h;
        tiles_.assign(static_cast<std::size_t>(w) * h, fill);
    }

    bool inBounds(int x, int y) const {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

    TileId at(int x, int y) const {
        return inBounds(x, y) ? tiles_[static_cast<std::size_t>(y) * width_ + x] : TileId(0);
    }
    void set(int x, int y, TileId v) {
        if (inBounds(x, y)) tiles_[static_cast<std::size_t>(y) * width_ + x] = v;
    }
    void fill(TileId v) { tiles_.assign(tiles_.size(), v); }

    int width() const { return width_; }
    int height() const { return height_; }

    // Contiguous access for serialization / procedural generation.
    TileId* data() { return tiles_.data(); }
    const TileId* data() const { return tiles_.data(); }
    std::size_t size() const { return tiles_.size(); }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<TileId> tiles_;
};

} // namespace ty
