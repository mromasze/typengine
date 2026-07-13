# Migration: Typengine v0.0.1 → v0.1.0

v0.1.0 removes the Duktape/JS scripting layer entirely and replaces the
singleton API with instance-based objects under `namespace ty`.

## Removed

- [x] `vendor/duktape/`, `src/scripting/`, `include/Typengine/scripting/` — no scripting layer
- [x] `include/vulkan/`, `include/vk_video/`, `src/shaders/` — dead code; the renderer
      is and was SDL_Renderer-based
- [x] `Typengine::UI` Duktape string callbacks — replaced by `std::function`
- [x] Singletons (`Engine::Get()`, `Renderer::Get()`, ...) — subsystems are owned
      by a `ty::Engine` instance
- [x] `std::vector<std::vector<int>>` map in `World` — replaced by flat `ty::TileGrid`

## API mapping

| v0.0.1 | v0.1.0 |
|--------|--------|
| `Typengine::Core::Engine::Get().Initialize(t, w, h)` | `ty::Engine engine; engine.initialize({.title=t, .width=w, .height=h})` |
| `Engine::Get().PollEvents()` + manual loop | `engine.run(std::make_unique<MyScene>())` (or manual: `pollEvents`/`beginFrame`/`endFrame`) |
| `Renderer::Get().DrawTexture(...)` | `engine.renderer().drawTexture(...)` |
| `Renderer::Get().LoadTexture(path)` | `engine.renderer().loadTexture(path)` |
| `Input::Get().IsKeyPressed(Key::W)` | `engine.input().isDown(ty::Key::W)`; edge: `wasPressed`/`wasReleased` |
| `World::Get().SetMapTile(x, y, v)` | `map.set(x, y, v)` on `ty::IsoTilemap` / `ty::TileGrid` |
| `World::Get().IsWalkable(x, y, w, h)` | `map.isAreaWalkable({x, y, w, h})` (tile units) |
| `World::Get().AddCollisionBox(...)` | entity `ty::Collider` + `ty::SpatialGrid` query |
| `UISystem::Get().AddButton(..., "jsCallback")` | `panel.addButton(name, rect, [] { ... })` |
| JS `Init`/`Update`/`Render` callbacks | `ty::Scene::onEnter`/`update`/`render` |
| `DrawNumber(...)` (JS binding) | `renderer.drawNumber(...)` |
| engine_config.txt hot-reload | kept — `EngineConfig::configFile` |

## Game-side checklist (expandy-dev)

- [x] Delete TS/JS toolchain: `package.json`, `tsconfig.json`, `src/*.ts`, `dist/`
- [x] Delete checked-in build artifacts (`Expandy_game*`, `run.log`)
- [x] Port `game.ts` logic to C++ scenes/systems (movement, stamina sprint,
      camera follow, pause menu)
- [x] Update submodule to the refactored engine commit
- [x] `add_subdirectory(Typengine)` + `target_link_libraries(expandy typengine::typengine)`
- [ ] Push both repos; verify `git clone --recurse-submodules` + clean build on CI
- [ ] Replace placeholder diamond tiles with a real isometric tileset
