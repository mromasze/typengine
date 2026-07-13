#include "engine/IsoTilemap.hpp"

#include <algorithm>

namespace ty {

void IsoTilemap::render(Renderer& renderer, const IsoCamera& camera, int layer) const {
    if (tileset_.texture == InvalidTexture || grid_.width() == 0) return;

    const float tileW = static_cast<float>(camera.tileWidth());
    const float tileH = static_cast<float>(camera.tileHeight());
    const float zoom = camera.getZoom();

    // Source cell may be taller than the ground diamond (walls, cliffs);
    // the extra height hangs *above* the diamond.
    const float srcW = static_cast<float>(tileset_.tileW);
    const float srcH = static_cast<float>(tileset_.tileH);
    const float dstW = tileW * zoom;
    const float dstH = srcH * (tileW / srcW) * zoom;
    const float overhang = dstH - tileH * zoom;

    // Conservative visible-tile range: unproject the four screen corners and
    // take the bounding box in tile coordinates.
    const float vw = static_cast<float>(renderer.width());
    const float vh = static_cast<float>(renderer.height());
    Vec2 corners[4] = {
        camera.screenToWorld({0.0f, 0.0f}),
        camera.screenToWorld({vw, 0.0f}),
        camera.screenToWorld({0.0f, vh}),
        camera.screenToWorld({vw, vh}),
    };
    float minX = corners[0].x, maxX = corners[0].x;
    float minY = corners[0].y, maxY = corners[0].y;
    for (int i = 1; i < 4; ++i) {
        minX = std::min(minX, corners[i].x);
        maxX = std::max(maxX, corners[i].x);
        minY = std::min(minY, corners[i].y);
        maxY = std::max(maxY, corners[i].y);
    }
    // Pad by 2 tiles so tall tiles poking in from outside still draw.
    int x0 = std::max(0, static_cast<int>(std::floor(minX)) - 2);
    int y0 = std::max(0, static_cast<int>(std::floor(minY)) - 2);
    int x1 = std::min(grid_.width(), static_cast<int>(std::ceil(maxX)) + 2);
    int y1 = std::min(grid_.height(), static_cast<int>(std::ceil(maxY)) + 2);

    // Row-major iteration already yields correct painter's order for tiles.
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            TileId id = grid_.at(x, y);
            if (id == 0) continue;

            int cell = id - 1;
            int cellX = (cell % tileset_.columns) * tileset_.tileW;
            int cellY = (cell / tileset_.columns) * tileset_.tileH;

            // worldToScreen of the tile's (x,y) corner is the *top* vertex of
            // the diamond; center horizontally and lift tall tiles by the overhang.
            Vec2 screen = camera.worldToScreen({static_cast<float>(x), static_cast<float>(y)});
            Rect dst = {screen.x - dstW * 0.5f, screen.y - overhang, dstW, dstH};

            renderer.drawTexture(tileset_.texture,
                                 {static_cast<float>(cellX), static_cast<float>(cellY), srcW, srcH},
                                 dst, layer);
        }
    }
}

} // namespace ty
