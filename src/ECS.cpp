#include "engine/ECS.hpp"

#include <algorithm>
#include <vector>

namespace ty {

void renderSprites(Registry& registry, Renderer& renderer, const IsoCamera& camera) {
    struct Item {
        const Transform* transform;
        const Sprite* sprite;
        float depth;
    };

    static std::vector<Item> items;
    items.clear();

    auto view = registry.view<const Transform, const Sprite>();
    for (auto [entity, transform, sprite] : view.each()) {
        if (sprite.texture == InvalidTexture) continue;
        items.push_back({&transform, &sprite, isoDepth(transform.position)});
    }

    std::sort(items.begin(), items.end(),
              [](const Item& a, const Item& b) {
                  if (a.sprite->layer != b.sprite->layer) return a.sprite->layer < b.sprite->layer;
                  return a.depth < b.depth;
              });

    const float zoom = camera.getZoom();
    // Sequential sub-layer indices keep the renderer's stable sort from
    // reordering what we just depth-sorted.
    int order = 0;
    for (const auto& item : items) {
        Vec2 screen = camera.worldToScreen(item.transform->position);
        float w = item.sprite->size.x * item.transform->scale.x * zoom;
        float h = item.sprite->size.y * item.transform->scale.y * zoom;
        Rect dst = {screen.x - item.sprite->origin.x * item.transform->scale.x * zoom,
                    screen.y - item.sprite->origin.y * item.transform->scale.y * zoom, w, h};
        renderer.drawTexture(item.sprite->texture, item.sprite->src, dst,
                             item.sprite->layer * 10000 + order);
        ++order;
    }
}

void applyVelocity(Registry& registry, float dt) {
    auto view = registry.view<Transform, const Velocity>();
    for (auto [entity, transform, velocity] : view.each()) {
        transform.position += velocity.value * dt;
    }
}

} // namespace ty
