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

struct Vector3{
    float x;
    float y;
    float z;
};
struct Transform{
    Vector3 scale={1.0f,1.0f,1.0f};
    Vector3 rotate={0.0f,0.0f,0.0f};
    Vector3 translate{0.0f,0.0f,0.0f};
};
