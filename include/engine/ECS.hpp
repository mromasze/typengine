#pragma once
// Typengine — ECS.hpp
// Thin layer over EnTT: aliases, core components shared by engine helpers,
// and an isometric sprite-rendering helper.
//
// Game code owns its registry:
//   ty::Registry registry;
//   auto player = registry.create();
//   registry.emplace<ty::Transform>(player, ty::Vec2{8.0f, 8.0f});
//   registry.emplace<ty::Sprite>(player, sprite);

#include <entt/entt.hpp>

#include "engine/IsoCamera.hpp"
#include "engine/Math.hpp"
#include "engine/Renderer.hpp"

namespace ty {

using Registry = entt::registry;
using Entity = entt::entity;
inline constexpr entt::null_t NullEntity = entt::null;

// --- Core components ------------------------------------------------------
// Position in world space (tile units — see IsoCamera.hpp for the convention).
struct Transform {
    Vec2 position{};
    Vec2 scale{1.0f, 1.0f};
};

struct Velocity {
    Vec2 value{};
};

// Drawable sprite. `size` is on-screen pixels at zoom 1; `origin` is the
// pixel inside that size that sits on Transform::position (the "feet" point).
struct Sprite {
    TextureId texture = InvalidTexture;
    Rect src{};                 // source rect in the texture (pixels)
    Vec2 size{};                // destination size (pixels at zoom 1)
    Vec2 origin{};              // anchor inside `size` placed at the world position
    int layer = 10;             // renderer layer; iso depth refines order inside it
};

// Axis-aligned collision box in world (tile) units, relative to the entity
// position. Insert `worldBounds(t.position)` into a SpatialGrid each frame.
struct Collider {
    Rect local{};               // offset + size relative to Transform::position
    bool solid = true;

    Rect worldBounds(Vec2 position) const {
        return {position.x + local.x, position.y + local.y, local.w, local.h};
    }
};

// --- Helpers ----------------------------------------------------------------
// Draws every entity with Transform + Sprite through the iso camera,
// depth-sorted so entities further "down" the diamond overlap correctly.
void renderSprites(Registry& registry, Renderer& renderer, const IsoCamera& camera);

// Applies Velocity to Transform (no collision). For collision-aware movement
// resolve against IsoTilemap::isAreaWalkable / SpatialGrid in game systems.
void applyVelocity(Registry& registry, float dt);

} // namespace ty
