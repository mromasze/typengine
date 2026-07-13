// typengine — hello-world example
// Opens a window, renders a procedurally-textured isometric checkerboard,
// moves an entity with WASD (Shift = sprint), camera follows.
// No external assets required.

#include <engine/Typengine.hpp>

#include <cmath>
#include <memory>
#include <vector>

namespace {

// Builds a tiny 2-cell tileset atlas at runtime so the example needs no files.
ty::TextureId makeCheckerAtlas(ty::Renderer&);

class HelloScene : public ty::Scene {
public:
    void onEnter(ty::Engine& engine) override {
        auto& renderer = engine.renderer();

        camera.setTileSize(64, 32);
        camera.setViewport(renderer.width(), renderer.height());
        camera.setPosition({MapSize / 2.0f, MapSize / 2.0f});

        ty::Tileset tileset;
        tileset.texture = makeCheckerAtlas(renderer);
        tileset.tileW = 64;
        tileset.tileH = 32;
        tileset.columns = 2;

        map.setTileset(tileset);
        map.resize(MapSize, MapSize);
        for (int y = 0; y < MapSize; ++y)
            for (int x = 0; x < MapSize; ++x)
                map.set(x, y, static_cast<ty::TileId>(1 + (x + y) % 2));
        map.setWalkable(1, true);
        map.setWalkable(2, true);

        player = registry.create();
        registry.emplace<ty::Transform>(player, ty::Vec2{MapSize / 2.0f, MapSize / 2.0f});
        ty::Sprite sprite;
        sprite.texture = renderer.createSolidTexture({230, 80, 80, 255});
        sprite.src = {0, 0, 1, 1};
        sprite.size = {24, 48};
        sprite.origin = {12, 48};   // feet at the world position
        registry.emplace<ty::Sprite>(player, sprite);
    }

    void update(ty::Engine& engine, float dt) override {
        auto& input = engine.input();
        if (input.wasPressed(ty::Key::Escape)) {
            engine.quit();
            return;
        }

        ty::Vec2 dir;
        if (input.isDown(ty::Key::W)) dir.y -= 1.0f;
        if (input.isDown(ty::Key::S)) dir.y += 1.0f;
        if (input.isDown(ty::Key::A)) dir.x -= 1.0f;
        if (input.isDown(ty::Key::D)) dir.x += 1.0f;

        auto& transform = registry.get<ty::Transform>(player);
        if (dir.lengthSq() > 0.0f) {
            float speed = input.isDown(ty::Key::Shift) ? 8.0f : 4.0f; // tiles/second
            ty::Vec2 next = transform.position + dir.normalized() * (speed * dt);
            if (map.isWalkable(next)) transform.position = next;
        }

        camera.setViewport(engine.screenWidth(), engine.screenHeight());
        camera.follow(transform.position, 8.0f, dt);
    }

    void render(ty::Engine& engine) override {
        auto& renderer = engine.renderer();
        map.render(renderer, camera, 0);
        ty::renderSprites(registry, renderer, camera);
    }

private:
    static constexpr int MapSize = 24;
    ty::Registry registry;
    ty::Entity player = ty::NullEntity;
    ty::IsoCamera camera;
    ty::IsoTilemap map;
};

// 128x32 atlas: two 64x32 diamond tiles (light / dark green), transparent corners.
ty::TextureId makeCheckerAtlas(ty::Renderer& renderer) {
    constexpr int W = 128, H = 32, TileW = 64, TileH = 32;
    std::vector<unsigned char> pixels(W * H * 4, 0);

    const unsigned char colors[2][4] = {{130, 170, 95, 255}, {95, 130, 70, 255}};
    for (int cell = 0; cell < 2; ++cell) {
        for (int y = 0; y < TileH; ++y) {
            // Diamond scanline: width grows to full at the middle row.
            float rowT = 1.0f - std::abs((y + 0.5f) / TileH * 2.0f - 1.0f);
            int half = static_cast<int>(rowT * TileW / 2.0f);
            for (int x = TileW / 2 - half; x < TileW / 2 + half; ++x) {
                int px = cell * TileW + x;
                unsigned char* p = &pixels[(y * W + px) * 4];
                p[0] = colors[cell][0];
                p[1] = colors[cell][1];
                p[2] = colors[cell][2];
                p[3] = colors[cell][3];
            }
        }
    }
    return renderer.createTexture(pixels.data(), W, H);
}

} // namespace

int main(int, char**) {
    ty::Engine engine;

    ty::EngineConfig config;
    config.title = "typengine — hello world";
    config.width = 1280;
    config.height = 720;
    config.vsync = true;

    if (!engine.initialize(config)) return 1;
    engine.run(std::make_unique<HelloScene>());
    return 0;
}
