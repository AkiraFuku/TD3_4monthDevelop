#include "BrokenBlock.h"
#include "ModelManager.h"
#include "imgui.h"
#include <numbers>

void BrokenBlock::Initialize(const Vector3& pos, float width, float depth)
{
    position_ = pos;
    this->width = width;
    height = depth;
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    ModelManager::GetInstance()->LoadModel("resources", "brokenBlock/brokenBlock.obj");
    object_->SetModel("brokenBlock/brokenBlock.obj");
    object_->SetTranslate(pos);
    float pi = std::numbers::pi_v<float>;
    object_->SetRotate({ 0.0f, pi, 0.0f });
    Vector3 scale = { this->width,1.0f,height };
    object_->SetScale(scale);
}

void BrokenBlock::Update()
{
    if (object_) {
        object_->Update();
    }

#ifdef USE_IMGUI
    ImGui::Begin("Broken block");

    ImGui::Text("Count: %d", currentCount_);
    ImGui::Text("MaxCount: %d", maxbreakCount_);

    ImGui::End();
#endif

}

void BrokenBlock::Draw()
{
    if (object_) {
        object_->Draw();
    }
}

bool BrokenBlock::IsInside(const Vector3& pos) const
{
    float minX = position_.x - (width / 2.0f);
    float maxX = position_.x + (width / 2.0f);
    float minZ = position_.z - (height / 2.0f);
    float maxZ = position_.z + (height / 2.0f);

    // 接続用のマージン（ここも重要）
    return (pos.x >= minX && pos.x <= maxX &&
        pos.z >= minZ && pos.z <= maxZ);
}

void BrokenBlock::CheckRiding(const Vector3& pos, const void* entityPtr) {

    if (isBroken_)
    {
        return;
    }

    // まだ乗っていないキャラだったら判定
    if (riders_.find(entityPtr) == riders_.end())
    {
        // ブロックにキャラが乗っていたら
        if (IsInside(pos))
        {
            currentCount_++;
            riders_.insert(entityPtr);
            
            if (currentCount_ >= maxbreakCount_)
            {
                isImpassable_ = true;
            }
        }
    }
    else
    {
        // 既に乗っているキャラが降りたら
        if (!IsInside(pos))
        {
            if (isImpassable_)
            {
                isBroken_ = true;
                return;
            }
        }
    }
}

bool BrokenBlock::IsRider(const void* entityPtr) const
{
    // riders_ (std::set) の中に自分がいれば true
    return riders_.find(entityPtr) != riders_.end();

}

BrokenBlock::AABB BrokenBlock::GetAABB() const
{
    return {
       { position_.x - width / 2.0f, 0, position_.z - height / 2.0f },
       { position_.x + width / 2.0f, 0, position_.z + height / 2.0f }
    };
}
