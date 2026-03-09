#include "Player.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Input.h"

void Player::Initialize()
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    ModelManager::GetInstance()->LoadModel("player/player.obj");
    object_->SetModel("player/player.obj");
}

void Player::Finalize()
{
    
}

void Player::Update()
{
  


    ImGui::Begin("Player Window");

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

    object_->Update();
}

void Player::Draw()
{
    object_->Draw();
}