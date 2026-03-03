#pragma once

namespace Typengine::Input {

enum class Key {
    W, A, S, D, SHIFT, ESC, UNKNOWN
};

class Input {
public:
    static Input& Get() { static Input instance; return instance; }

    void Update();
    bool IsKeyPressed(Key k);
    
    
    void GetMousePos(int& x, int& y);
    bool IsMouseButtonPressed(int button); 

private:
    Input() = default;
    int mouseX = 0, mouseY = 0;
    unsigned int mouseState = 0;
};

} 
