#include "CollisionMask.h"

bool CollisionMask::IsWall(float x, float z) const
{
    int ix = static_cast<int>(x);
    int iz = static_cast<int>(z);

    // 画面外は壁として扱う
    if (ix < 0 || ix >= widthX || iz < 0 || iz >= widthZ) return true;

    // インデックス計算（1ピクセル1バイトの場合）
    return data[iz * widthX + ix] < 128; // 閾値で判定
}