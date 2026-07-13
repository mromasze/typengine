# Typengine

A small, MIT-licensed 2D game engine in C++20, built for isometric pixel-art
games. SDL2 rendering, EnTT entity-component-system, isometric tilemaps with
camera projection, and uniform-grid spatial partitioning — no scripting layer,
pure C++ API.

## Features

- **SDL2 renderer** — layer-sorted draw queue, texture atlas draws,
  filled rects, procedural textures, nearest-neighbor scaling for pixel art
- **EnTT ECS** — `ty::Registry` alias plus core components
  (`Transform`, `Velocity`, `Sprite`, `Collider`) and an iso-depth-sorted
  sprite renderer
- **Isometric tilemap** — flat contiguous tile storage (`TileGrid`),
  tileset atlases, viewport-culled 2:1 diamond rendering, per-tile walkability
- **IsoCamera** — world↔screen isometric projection, zoom, smooth follow
- **Spatial partitioning** — header-only uniform grid (`ty::SpatialGrid<T>`)
  for broad-phase collision queries
- **Input** — keyboard/mouse polling with per-frame edge detection
  (`isDown` / `wasPressed` / `wasReleased`)
- **Scenes** — minimal `ty::Scene` interface with a deferred scene switch
- **UI** — simple buttons with `std::function` callbacks
- **Config hot-reload** — optional `key=value` file watched at runtime
  (`vsync`, `fps_limit`)

## Dependencies

All fetched automatically via CMake `FetchContent` when not installed:

| Library | License | Used for |
|---------|---------|----------|
| [SDL2](https://github.com/libsdl-org/SDL) | zlib | window, rendering, input |
| [EnTT](https://github.com/skypjack/entt) | MIT | ECS |
| [stb_image](https://github.com/nothings/stb) (vendored) | public domain / MIT | image loading |

No GPL dependencies.

## Building

```sh
cmake -B build
cmake --build build
```

Produces the static library (`libtypengine.a` / `typengine.lib`) and, when
built as the top-level project, the examples. Run the demo:

```sh
./build/examples/hello-world/hello-world
```

Works on Windows (MSVC, MinGW), Linux and macOS.

## Using in your game

### As a subdirectory (recommended, e.g. git submodule)

```cmake
add_subdirectory(typengine)
target_link_libraries(my_game PRIVATE typengine::typengine)
```

### As an installed package

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX=/your/prefix
cmake --build build --target install
```

```cmake
find_package(typengine CONFIG REQUIRED)
target_link_libraries(my_game PRIVATE typengine::typengine)
```

## API overview

Everything lives in `namespace ty`. Include the umbrella header or individual
ones from `include/engine/`:

```cpp
#include <engine/Typengine.hpp>
```

### Coordinate convention

Gameplay ("world") space is **cartesian, in tile units** — movement and
collision are plain axis-aligned math. `ty::IsoCamera` projects world space to
the screen with the classic 2:1 diamond:

```
screen.x = (world.x - world.y) * tileWidth  / 2
screen.y = (world.x + world.y) * tileHeight / 2
```

Entity depth ordering uses `ty::isoDepth(pos) = pos.x + pos.y`.

### Minimal game

```cpp
#include <engine/Typengine.hpp>

class MyScene : public ty::Scene {
    ty::Registry registry;
    ty::IsoCamera camera{64, 32};
    ty::IsoTilemap map;
    ty::Entity player = ty::NullEntity;

public:
    void onEnter(ty::Engine& engine) override {
        ty::Tileset tiles{engine.renderer().loadTexture("assets/tiles.png"),
                          64, 32, /*columns=*/8};
        map.setTileset(tiles);
        map.resize(64, 64, /*fill=*/1);
        map.setWalkable(1, true);

        player = registry.create();
        registry.emplace<ty::Transform>(player, ty::Vec2{32.0f, 32.0f});
        registry.emplace<ty::Sprite>(player, ty::Sprite{
            engine.renderer().loadTexture("assets/player.png"),
            {0, 0, 32, 32},   // src rect
            {48, 48},         // on-screen size
            {24, 44},         // origin ("feet")
            /*layer=*/10});
    }

    void update(ty::Engine& engine, float dt) override {
        auto& in = engine.input();
        ty::Vec2 dir;
        if (in.isDown(ty::Key::W)) dir.y -= 1;
        if (in.isDown(ty::Key::S)) dir.y += 1;
        if (in.isDown(ty::Key::A)) dir.x -= 1;
        if (in.isDown(ty::Key::D)) dir.x += 1;

        auto& t = registry.get<ty::Transform>(player);
        ty::Vec2 next = t.position + dir.normalized() * (4.0f * dt);
        if (map.isWalkable(next)) t.position = next;

        camera.setViewport(engine.screenWidth(), engine.screenHeight());
        camera.follow(t.position, 8.0f, dt);
    }

    void render(ty::Engine& engine) override {
        map.render(engine.renderer(), camera);
        ty::renderSprites(registry, engine.renderer(), camera);
    }
};

int main(int, char**) {
    ty::Engine engine;
    if (!engine.initialize({.title = "My Game", .width = 1280, .height = 720}))
        return 1;
    engine.run(std::make_unique<MyScene>());
    return 0;
}
```

### Collision broad-phase

```cpp
ty::SpatialGrid<ty::Entity> grid(2.0f);   // 2x2-tile cells

grid.clear();
registry.view<ty::Transform, ty::Collider>().each(
    [&](ty::Entity e, ty::Transform& t, ty::Collider& c) {
        grid.insert(e, c.worldBounds(t.position));
    });

grid.query(attackArea, [&](ty::Entity hit) { /* narrow phase / damage */ });
```

### Headers

| Header | Contents |
|--------|----------|
| `engine/Engine.hpp` | `Engine`, `EngineConfig`, `Scene` |
| `engine/Renderer.hpp` | `Renderer`, `TextureId` |
| `engine/Input.hpp` | `Input`, `Key`, `MouseButton` |
| `engine/ECS.hpp` | EnTT aliases, core components, `renderSprites` |
| `engine/IsoCamera.hpp` | `IsoCamera`, projection helpers |
| `engine/IsoTilemap.hpp` | `IsoTilemap`, `Tileset` |
| `engine/Tilemap.hpp` | `TileGrid` (flat tile storage) |
| `engine/SpatialGrid.hpp` | `SpatialGrid<T>` |
| `engine/UI.hpp` | `UIPanel`, `UIButton` |
| `engine/Math.hpp` | `Vec2`, `Rect`, `Color`, helpers |
| `engine/Typengine.hpp` | umbrella header |

## License

MIT — see [LICENSE](LICENSE).
