#include "EggExplosion.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include <numbers>
#include <random>
#include "MathFunction.h"

void EggExplosion::Initialize(const Vector3& pos)
{
    ModelManager::GetInstance()->LoadModel("resources", "plane.obj");
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("plane.obj");

    // ランダムエンジンの初期化
    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distTime(1.0f, 10.0f);

    Vector3 randamTranslate = { distribution(randomEngine),distribution(randomEngine) ,distribution(randomEngine) };
    translate_ = pos + randamTranslate;
    object_->SetTranslate(translate_);
    Vector3 randamScale = { distribution(randomEngine) + 1.0f, distribution(randomEngine) + 1.0f, distribution(randomEngine) + 1.0f };
    object_->SetScale(randamScale);
    float pi = std::numbers::pi_v<float>;
    rotate_ = { 0.0f, -pi, 0.0f };
    object_->SetRotate(rotate_);
    velocity_ = { distribution(randomEngine) * 0.1f, distribution(randomEngine) * 0.1f, distribution(randomEngine) * 0.1f };

    object_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    a = object_->GetColor().w;
    object_->SetBlendMode(BlendMode::Add);
}

void EggExplosion::Finalize()
{
}

void EggExplosion::Update()
{
#ifdef USE_IMGUI

    ImGui::Begin("Egg Explosion");
    ImGui::DragFloat3("pos", &translate_.x, 1.0f, 0.0f, 500.0f);
    ImGui::DragFloat3("scale", &scale_.x, 1.0f, 0.0f, 500.0f);
    ImGui::DragFloat3("rotate", &rotate_.x, 1.0f, 0.0f, 360.0f);
    ImGui::End();

#endif

    // 位置を更新
    translate_ = Add(translate_, velocity_);

    if (scale_.x > 0.01f && scale_.y > 0.01f && scale_.z > 0.01f)
    {
        scale_ = Subtract(scale_, Vector3{ 0.01f, 0.01f, 0.01f });
        object_->SetScale(scale_);
    }

    a -= 0.01f; // 徐々に透明にする
    object_->SetModelInstanceColor("egg", { 1.0f, 1.0f, 1.0f, a });

    object_->SetTranslate(translate_);
    object_->Update();
}

void EggExplosion::Draw()
{
    object_->Draw();
}
