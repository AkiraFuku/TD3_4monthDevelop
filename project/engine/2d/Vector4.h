#pragma once
#include <vector>
#include <string>

struct Vector4{
    float x;
    float y;
    float z;
    float w;
};
struct Matrix4x4{
   float m[4][4];
};
struct Matrix3x3{
   float m[3][3];
};

struct Transform{
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};
