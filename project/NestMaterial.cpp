#include "NestMaterial.h"
#include "ModelManager.h"
#include "imgui.h"

void NestMaterial::Initialize(const Vector3& pos)
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    ModelManager::GetInstance()->LoadModel("resources", "nestMaterial/nestMaterial.obj");
    object_->SetModel("nestMaterial/nestMaterial.obj");

    object_->SetTranslate(pos);
}

void NestMaterial::Finalize()
{

}

void NestMaterial::Update()
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
}

void NestMaterial::Draw()
{
    if (isDead_)
    {
        return;
    }

    object_->Draw();
}

void NestMaterial::OnCollision()
{

    // プレイヤーが接触していたらデスフラグを立てる
    isDead_ = true;
}

Vector3 NestMaterial::GetWorldPosition() const
{
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos = object_->GetTranslate();
    return worldPos;
}

AABB NestMaterial::GetAABB() const
{
    Vector3 worldPos = GetWorldPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}
