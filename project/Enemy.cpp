#include "Enemy.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Input.h"

void Enemy::Initialize(const Vector3& pos)
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    translate_ = pos;
    object_->SetTranslate(translate_);

    ModelManager::GetInstance()->LoadModel("axis.obj");
    object_->SetModel("axis.obj");
}

void Enemy::Finalize()
{

}

void Enemy::Update()
{
    ImGui::Begin("Enemy Window");

    Vector3 scale = object_->GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f))
    {
        object_->SetScale(scale);
    }

    Vector3 rotate = object_->GetRotate();
    if (ImGui::DragFloat3("Rotate", &rotate.x, 0.1f, -360.0f, 360.0f))
    {
        object_->SetRotate(rotate);
    }

    Vector3 translate = object_->GetTranslate();
    if (ImGui::DragFloat3("Translate", &translate.x, 0.1f, -100.0f, 100.0f))
    {
        object_->SetTranslate(translate);
    }


    ImGui::End();

    // 経路があれば移動する
    if (!path_.empty()) {
        Vector3 currentPos = object_->GetTranslate();
        Vector3 nextTarget = GridToWorld(path_.front());

        // 目標へのベクトル
        Vector3 diff = { nextTarget.x - currentPos.x, 0.0f, nextTarget.z - currentPos.z };
        float distance = std::sqrt(diff.x * diff.x + diff.z * diff.z);

        if (distance < 0.2f) {
            // 到着したら次のポイントへ
            path_.pop_front();
        }
        else {
            // 移動実行
            currentPos.x += (diff.x / distance) * moveSpeed_;
            currentPos.z += (diff.z / distance) * moveSpeed_;
            object_->SetTranslate(currentPos);
        }
    }

    object_->Update();
}

void Enemy::Draw()
{
    object_->Draw();
}

Vector3 Enemy::GetWorldPosition() const
{
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos = object_->GetTranslate();
    return worldPos;
}

AABB Enemy::GetAABB() const
{
    Vector3 worldPos = GetWorldPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}

Point Enemy::WorldToGrid(const Vector3& pos) {
    // あなたの CollisionMask::IsWall 内の u, v 計算ロジックをここに移植します
    // ここでは単純なキャストの例を示します
    return { (int)std::floor(pos.x), (int)std::floor(pos.z) };
}

Vector3 Enemy::GridToWorld(const Point& grid) {
    // グリッドの中心を歩くように +0.5f 調整
    return { (float)grid.x + 0.5f, 0.0f, (float)grid.y + 0.5f };
}

void Enemy::RecalculatePath(const Vector3& eggPos)
{
    // 1. 現在地と目的地をピクセル単位の座標に変換
    Point start = WorldToGrid(object_->GetTranslate());
    Point goal = WorldToGrid(eggPos);

    // 2. CollisionMaskの状態に基づいて経路を計算
    // ここで渡す map 引数は、PathFinder内部で CollisionMask::IsWall を呼ぶように改造している前提、
    // もしくはダミーを渡し、PathFinder内で CollisionMask を直接参照させます。
    // 今回は PathFinder が CollisionMask を直接見に行く方式を想定し、引数を調整します。

    // マップの最大幅を取得（CollisionMaskの実装に合わせる）
    // 本来は CollisionMask から widthX, widthZ を取得できるゲッターがあると理想的です。
    int w = 512; // 仮の数値。実際のMask.pngの解像度に合わせてください
    int h = 512;

    std::vector<Point> newPath = PathFinder::FindPath(start, goal, nullptr, w, h);

    // 3. 経路を更新
    path_.clear();
    if (!newPath.empty()) {
        for (const auto& p : newPath) {
            path_.push_back(p);
        }
        // 現在地点のマスは削除
        path_.pop_front();
    }
}

