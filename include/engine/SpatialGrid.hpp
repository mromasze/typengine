#pragma once
// Typengine — SpatialGrid.hpp
// Uniform-grid spatial partitioning for broad-phase collision queries.
// Header-only. Rebuild each frame: clear() + insert() all movers, then query().
//
// Typical use with the ECS:
//   ty::SpatialGrid<ty::Entity> grid(2.0f);           // cell = 2 tiles
//   grid.clear();
//   registry.view<Transform, Collider>().each([&](auto e, auto& t, auto& c) {
//       grid.insert(e, c.worldBounds(t.position));
//   });
//   grid.query(attackArea, [&](ty::Entity hit) { ... });

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/Math.hpp"

namespace ty {

template <typename T>
class SpatialGrid {
public:
    explicit SpatialGrid(float cellSize = 4.0f) : cellSize_(cellSize <= 0.0f ? 1.0f : cellSize) {}

    void clear() {
        for (auto& [key, bucket] : cells_) bucket.clear();
        count_ = 0;
    }

    void insert(T id, const Rect& aabb) {
        forEachCell(aabb, [&](std::int64_t key) { cells_[key].push_back({id, aabb}); });
        ++count_;
    }

    // Calls fn(id) once per distinct entry whose AABB overlaps `area`.
    template <typename Fn>
    void query(const Rect& area, Fn&& fn) const {
        std::unordered_set<T> seen;
        forEachCell(area, [&](std::int64_t key) {
            auto it = cells_.find(key);
            if (it == cells_.end()) return;
            for (const auto& entry : it->second) {
                if (entry.aabb.intersects(area) && seen.insert(entry.id).second) {
                    fn(entry.id);
                }
            }
        });
    }

    std::size_t size() const { return count_; }
    float cellSize() const { return cellSize_; }

private:
    struct Entry {
        T id;
        Rect aabb;
    };

    template <typename Fn>
    void forEachCell(const Rect& aabb, Fn&& fn) const {
        int x0 = static_cast<int>(std::floor(aabb.x / cellSize_));
        int y0 = static_cast<int>(std::floor(aabb.y / cellSize_));
        int x1 = static_cast<int>(std::floor((aabb.x + aabb.w) / cellSize_));
        int y1 = static_cast<int>(std::floor((aabb.y + aabb.h) / cellSize_));
        for (int cy = y0; cy <= y1; ++cy)
            for (int cx = x0; cx <= x1; ++cx)
                fn(cellKey(cx, cy));
    }

    static std::int64_t cellKey(int cx, int cy) {
        return (static_cast<std::int64_t>(cx) << 32) ^ static_cast<std::uint32_t>(cy);
    }

    float cellSize_;
    std::size_t count_ = 0;
    std::unordered_map<std::int64_t, std::vector<Entry>> cells_;
};

} // namespace ty
