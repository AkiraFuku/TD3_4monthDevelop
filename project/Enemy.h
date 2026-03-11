#pragma once

#include"Object3D.h"
#include "Model.h"
#include "Camera.h"
#include "DrawFunction.h"
#include "PathFinder.h" 
#include <deque>
#include <memory>

class Player;

class Enemy
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

    // 経路探索関数
    void RecalculatePath(const Vector3& eggPos);

    // --- 座標変換ユーティリティ ---
    Point WorldToGrid(const Vector3& pos);
    Vector3 GridToWorld(const Point& grid);

    
public: // 外部入出力





private:

    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = { 1.0f,1.0f,1.0f };
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };
    Vector3 translate_ = { 0.0f,0.0f,0.0f };

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 経路データ
    std::deque<Point> path_;

    // 移動パラメータ
    float moveSpeed_ = 0.1f;

};

