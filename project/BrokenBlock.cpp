#include "BrokenBlock.h"
#include "ModelManager.h"
#include "imgui.h"
#include <numbers>

void BrokenBlock::Initialize(const Vector3& pos, float width, float depth)
{
    position_ = pos;
    this->width = width;
    height = depth;



    ModelManager::GetInstance()->LoadModel("resources", "brokenBlock/1/brokenBlock.obj");
    ModelManager::GetInstance()->LoadModel("resources", "brokenBlock/2/brokenBlock.obj");

    for (int i=0; i < maxbreakCount_; i++)
    {
        std::unique_ptr object = std::make_unique<Object3d>();
        object->Initialize();
        object->SetModel("brokenBlock/" + std::to_string(i + 1) + "/brokenBlock.obj");
        object->SetTranslate(pos);
        float pi = std::numbers::pi_v<float>;
        object->SetRotate({ 0.0f, pi, 0.0f });
        Vector3 scale = { this->width,1.0f,height };
        object->SetScale(scale);
        objects_.push_back(std::move(object));
    }

    broken_ = Audio::GetInstance()->LoadAudio("resources/sounds/broken.wav");
}

void BrokenBlock::Update()
{
    for (const std::unique_ptr <Object3d>& object : objects_)
    {
        object->Update();
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
    int num = maxbreakCount_ - currentCount_ - 1;
    if (num >= 0)
    {
        objects_[num]->Draw();
    }
    else
    {
        objects_[0]->Draw();
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

    // 1. まず、このキャラが現在ブロックの「内側」にいるか「外側」にいるかをチェック
    if (IsInside(pos))
    {
        // まだ登録されていない新しいキャラなら、ここで初めて「乗った」とみなす
        if (riders_.find(entityPtr) == riders_.end())
        {
            riders_.insert(entityPtr);
            currentCount_++; // 🌟ここで初めてカウントが増える（重複しない）

            // 耐久度が限界に達したら、新規進入を禁止するフラグを立てる
            if (currentCount_ >= maxbreakCount_)
            {
                isImpassable_ = true;
            }
        }
    }
    else
    {
        // 外側にいる（降りた）場合、もし登録されていたらリストから削除する
        // 🌟その場ですぐに崩落させるのではなく、まずはリストからの抹消だけを行う
        if (riders_.find(entityPtr) != riders_.end())
        {
            riders_.erase(entityPtr);
        }
    }

    // 2. 【ここが超重要】
    // 「耐久度が限界（isImpassable_）」であり、かつ「乗っているキャラが誰もいなくなった（riders_.empty()）」のなら、
    // その瞬間に初めて完全に崩落（isBroken_）させる！
    if (isImpassable_ && riders_.empty())
    {
        isBroken_ = true;
        // サウンド再生
        Audio::GetInstance()->PlayAudio(broken_, false, 1.0f);
        return;
    }
}

bool BrokenBlock::IsRider(const void* entityPtr) const
{
    // riders_ (std::set) の中に自分がいれば true
    return riders_.find(entityPtr) != riders_.end();

}

AABB BrokenBlock::GetAABB() const
{
    return {
       { position_.x - width / 2.0f, 0, position_.z - height / 2.0f },
       { position_.x + width / 2.0f, 0, position_.z + height / 2.0f }
    };
}
