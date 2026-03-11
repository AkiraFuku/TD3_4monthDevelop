#include "Enemy.h"
#include "ModelManager.h"

void Enemy::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("axis.obj"); // 敵のモデル
    object_->SetTranslate(pos);
}

void Enemy::RecalculatePath(const Vector3& eggPos, ThreadManager* tm) {
    Point start = WorldToGrid(object_->GetTranslate());
    Point goal = WorldToGrid(eggPos);

    // PathFinderにThreadManagerを託す
    std::vector<Point> newPath = PathFinder::FindPath(start, goal, 512, 512, tm);

    path_.clear();
    for (const auto& p : newPath) {
        path_.push_back(p);
    }
    if (!path_.empty()) path_.pop_front();
}

void Enemy::Update(const Vector3& eggPos, ThreadManager* tm) {
    // 定期的に経路を更新（糸が増えたり卵が動いたりするため）
    recalculateTimer_++;
    if (recalculateTimer_ > 60) {
        RecalculatePath(eggPos, tm);
        recalculateTimer_ = 0;
    }

    // 移動処理
    if (!path_.empty()) {
        Vector3 currentPos = object_->GetTranslate();
        Vector3 nextTarget = GridToWorld(path_.front());

        Vector3 diff = { nextTarget.x - currentPos.x, 0.0f, nextTarget.z - currentPos.z };
        float distance = std::sqrt(diff.x * diff.x + diff.z * diff.z);

        if (distance < 0.2f) {
            path_.pop_front();
        }
        else {
            currentPos.x += (diff.x / distance) * moveSpeed_;
            currentPos.z += (diff.z / distance) * moveSpeed_;
            object_->SetTranslate(currentPos);
        }
    }
    object_->Update();
}

void Enemy::Draw() { object_->Draw(); }

Point Enemy::WorldToGrid(const Vector3& pos) {
    return { (int)std::floor(pos.x), (int)std::floor(pos.z) };
}

Vector3 Enemy::GridToWorld(const Point& grid) {
    return { (float)grid.x + 0.5f, 0.0f, (float)grid.y + 0.5f };
}