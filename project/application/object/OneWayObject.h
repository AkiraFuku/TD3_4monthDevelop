#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include <cmath>
#include <numbers>
#include "DrawFunction.h"

class OneWayObject
{
public:
    // 方向の定義
    enum class Direction
    {
        PositiveX, // +X方向のみ許可
        NegativeX, // -X方向のみ許可
        PositiveZ, // +Z方向のみ許可
        NegativeZ  // -Z方向のみ許可
    };

    void Initialize(const Vector3& pos, Direction dir, float width, float depth);

    void Update();
    void Draw();

    // 進入可能か判定する
    // moveDir: 進入しようとしている移動ベクトル
    // currentPos: 進入しようとしている物体の座標
    bool CanPass(const Vector3& moveDir, const Vector3& currentPos) const;

    bool IsInside(const Vector3& pos) const;

    // プレイヤーの現在位置と移動量を受け取り、進入不可なら移動量を補正する
    void ResolveCollision(Vector3& currentPos, Vector3& moveVel) const;

    void DebugDrawImGui(int index, const Vector3& playerPos, const Vector3& playerMoveVel) const;

    bool IsAtEntrySide(const Vector3& playerPos) const;

public: // 外部入出力

    // ゲッター
    Vector3 GetPosition() const { return position_; }
    Direction GetDirection() const { return allowedDir_; }
    AABB GetAABB() const;

    // セッター
    void SetTranslate(const Vector3& translate)
    {
        position_ = translate;
        if (object_) {
            object_->SetTranslate(translate);
        }
    }

private:
    Vector3 position_;
    Direction allowedDir_;
    float width_;
    float depth_;

    // 3Dオブジェクト
    std::unique_ptr<Object3d> object_;
};