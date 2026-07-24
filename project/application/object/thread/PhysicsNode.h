#pragma once

#include <Vector4.h>

struct PhysicsNode {
  Vector3 currentPos;  // 今どこにいるか
  Vector3 previousPos; // 1フレーム前にどこにいたか
  bool isFixed;        // 壁などに固定されているフラグ
};