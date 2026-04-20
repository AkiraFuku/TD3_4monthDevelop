#pragma once

#include"Object3D.h"
#include "Vector3.h"
#include "Model.h"
#include "Camera.h"
#include "DrawFunction.h"
#include "Audio.h"

class NestMaterial
{
public:

    void Initialize(const Vector3& pos);

    void Finalize();

    void Update();

    void Draw();

    // ワールド座標を取得
    Vector3 GetWorldPosition() const;

    // AABBを取得
    AABB GetAABB() const;

    // 衝突応答
    void OnCollision();

    // デスフラグのgetter
    bool IsDead() { return isDead_; }

private:

    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = { 1.0f,1.0f,1.0f };
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };
    Vector3 translate_ = { 0.0f,0.0f,0.0f };

    // デスフラグ
    bool isDead_ = false;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // サウンド
    Audio::SoundHandle get_ = 0;

};

