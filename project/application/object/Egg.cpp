#include "Egg.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Input.h"

void Egg::Initialize()
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    ModelManager::GetInstance()->LoadModel("axis.obj");
    object_->SetModel("axis.obj");
}

void Egg::Finalize()
{

}

void Egg::Update()
{
    ImGui::Begin("Egg Window");

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

    Vector3 translates = object_->GetTranslate();

    // ゴール判定確認用の移動処理
    if (Input::GetInstance()->PushedKeyDown(DIK_UP))
    {
        // 奥に進む
        translates.z += 0.1f;
    }
    else if (Input::GetInstance()->PushedKeyDown(DIK_DOWN))
    {
        // 手前に進む
        translates.z -= 0.1f;
    }

    object_->SetTranslate(translates);

    object_->Update();
}

void Egg::Draw()
{
    object_->Draw();
}

Vector3 Egg::GetWorldPosition()
{
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos = object_->GetTranslate();
    return worldPos;
}
