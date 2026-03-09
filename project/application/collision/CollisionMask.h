#pragma once
#include "MathFunction.h"

class CollisionMask
{
public: 

    std::vector<uint8_t> data;
    int widthX, widthZ;

    // 特定座標が壁かどうかを判定
    bool IsWall(float x, float z) const;
};

