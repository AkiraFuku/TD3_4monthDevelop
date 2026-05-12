#pragma once
#include "Object3d.h"
#include "Camera.h"
#include "DrawFunction.h"
#include <random>


class EggExplosion
{
public:

    void Initialize(const Vector3& pos);

    void Finalize();

    void Update();

    void Draw();

private:

    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = { 1.0f,1.0f,0.0f };
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };
    Vector3 translate_ = { 0.0f,0.0f,0.0f };
    Vector3 velocity_ = { 0.0f,0.0f,0.0f };
    // 透明度
    float a = 1.0f;

    // 乱数生成器
    std::random_device seedGenerator;
    std::mt19937 randomEngine;
};

