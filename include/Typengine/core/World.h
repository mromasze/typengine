#pragma once
#include <vector>

namespace Typengine::Core {

struct CollisionBox {
    float x, y, w, h;
};

class World {
public:
    static World& Get() { static World instance; return instance; }

    void SetMapDimensions(int w, int h, int tileSize);
    void SetMapTile(int x, int y, int value);
    int GetMapTile(int x, int y) const;
    
    void AddCollisionBox(float x, float y, float w, float h);
    void ClearCollisionBoxes();

    bool IsWalkable(float x, float y, float w, float h, int walkableTileValue = 1);

private:
    World() = default;
    int mapW = 0;
    int mapH = 0;
    int tileSize = 32;
    std::vector<std::vector<int>> map;
    std::vector<CollisionBox> collisionBoxes;
};

} 
