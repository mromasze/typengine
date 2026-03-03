#include "Typengine/core/World.h"
#include <cmath>

namespace Typengine::Core {

void World::SetMapDimensions(int w, int h, int ts) {
    mapW = w;
    mapH = h;
    tileSize = ts;
    map.assign(w, std::vector<int>(h, 0));
}

void World::SetMapTile(int x, int y, int value) {
    if (x >= 0 && x < mapW && y >= 0 && y < mapH) {
        map[x][y] = value;
    }
}

int World::GetMapTile(int x, int y) const {
    if (x >= 0 && x < mapW && y >= 0 && y < mapH) {
        return map[x][y];
    }
    return -1;
}

void World::AddCollisionBox(float x, float y, float w, float h) {
    collisionBoxes.push_back({x, y, w, h});
}

void World::ClearCollisionBoxes() {
    collisionBoxes.clear();
}

bool World::IsWalkable(float x, float y, float w, float h, int walkableTileValue) {
    auto checkPoint = [&](float px, float py) {
        int tx = static_cast<int>(std::floor(px / tileSize));
        int ty = static_cast<int>(std::floor(py / tileSize));
        if (tx < 0 || tx >= mapW || ty < 0 || ty >= mapH) return false;
        return map[tx][ty] == walkableTileValue;
    };
    
    
    if (!checkPoint(x, y)) return false;
    if (!checkPoint(x + w, y)) return false;
    if (!checkPoint(x, y + h)) return false;
    if (!checkPoint(x + w, y + h)) return false;
    
    
    for (const auto& box : collisionBoxes) {
        if (x < box.x + box.w && x + w > box.x &&
            y < box.y + box.h && y + h > box.y) {
            return false;
        }
    }
    
    return true;
}

} 
