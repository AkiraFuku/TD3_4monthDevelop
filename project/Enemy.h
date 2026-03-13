#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "PathFinder.h"
#include <deque>
#include <memory>

class ThreadManager;

class Enemy {
public:
    void Initialize(const Vector3& pos);

    // GameSceneから必要な情報を毎フレーム受け取る
    void Update(const Vector3& eggPos, ThreadManager* tm);

    void Draw();

    // 経路探索
    void RecalculatePath(const Vector3& eggPos, ThreadManager* tm);

    // 経路をクリア
    bool IsPathClear(const Vector3& start, const Vector3& end, ThreadManager* tm);

private:
    Point WorldToGrid(const Vector3& pos);
    Vector3 GridToWorld(const Point& grid);

private:
    std::unique_ptr<Object3d> object_;
    std::deque<Point> path_;
    float moveSpeed_ = 0.08f;
    int recalculateTimer_ = 0;
};