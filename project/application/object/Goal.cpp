#include "Goal.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Egg.h"
#include "Player.h"
#include "SceneManager.h"
#include "GameScene.h"

void Goal::Initialize(const Vector3& pos)
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    ModelManager::GetInstance()->LoadModel("resources","goal/goal.obj");
    object_->SetModel("goal/goal.obj");

    object_->SetTranslate(pos);

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("resources/icon/shortage.png");
    sprite_->SetPosition({ 340.0f, 160.0f });
}

void Goal::Finalize()
{

}

void Goal::Update()
{
#ifdef USE_IMGUI
    ImGui::Begin("Goal Window");

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
#endif

    object_->Update();
    sprite_->Update();
}

void Goal::Draw()
{
    object_->Draw();

    if (isWarning_)
    {
        sprite_->Draw();
    }
}

void Goal::Clear()
{
    // 卵とゴールの当たり判定
    Vector3 goalPos = object_->GetTranslate();
    Vector3 eggPos = egg_->GetWorldPosition();

    // 卵がゴールの真上にあったら
    if (goalPos.x - 1.0f < eggPos.x && eggPos.x < goalPos.x + 1.0f &&
        goalPos.z - 1.0f < eggPos.z && eggPos.z < goalPos.z + 1.0f) {
        // プレイヤーの素材の回収数が必要数を下回っていたら警告
        if (player_->GetNestMaterial() < needNestMaterialCount_ || !egg_->IsOnPlayer())
        {
            isWarning_ = true;
        }
        else
        {
            // クリアフラグを立てる
            gameScene_->SetClear(true);
        }
    }
    else
    {
        isWarning_ = false;
    }
}
