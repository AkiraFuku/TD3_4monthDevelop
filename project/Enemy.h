#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "PathFinder.h"
#include "DrawFunction.h"
#include <deque>
#include <memory>
#include "OneWayObject.h"
#include "BrokenBlock.h"

class ThreadManager;
class Egg;

class Enemy {
public:
    void Initialize(const Vector3& pos);

    // GameSceneから必要な情報を毎フレーム受け取る
    void Update(const Vector3& eggPos, ThreadManager* tm, 
        const std::vector<std::unique_ptr<OneWayObject>>& oneWays, const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock);

    void Draw();

    // 経路探索
    void RecalculatePath(const Vector3& eggPos, ThreadManager* tm, 
        const std::vector<std::unique_ptr<OneWayObject>>& oneWays, const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock);

    // 経路をクリア
    bool IsPathClear(const Vector3& start, const Vector3& end, ThreadManager* tm);

    // ワールド座標を取得
    Vector3 GetWorldPosition() const;

    // AABBを取得
    AABB GetAABB() const;

    // 衝突応答
    void OnCollision(Egg* egg_);

    // ヒットフラグをセット
    void SetHitFlag(bool isHit) { isHit_ = isHit; }

    void SetPosition(const Vector3& pos);

    // 外部から強制的に再計算させる
    void RequestPathReplan() { shouldReplanNextUpdate_ = true; }

    bool GetCanMove()const { return canMove_; }

private:
    Point WorldToGrid(const Vector3& pos);
    Vector3 GridToWorld(const Point& grid);

private:
    std::unique_ptr<Object3d> object_;
    std::deque<Point> path_;
    float moveSpeed_ = 0.08f;
    int recalculateTimer_ = 0;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 卵への攻撃タイマー
    int attackTimer_ = 60;

    // 当たり判定フラグ
    bool isHit_ = false;
    bool canMove_ = true;

    bool shouldReplanNextUpdate_ = false; // 再計算予約フラグ

};