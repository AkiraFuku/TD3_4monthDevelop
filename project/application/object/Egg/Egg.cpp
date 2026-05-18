#include "Egg.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Input.h"
#include "Player.h"
#include "CollisionMask.h"
#include "SceneManager.h"
#include "GameScene.h"

void Egg::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();


    ModelManager::GetInstance()->LoadModel("resources", "egg/egg.obj");
    object_->AddModel("egg/egg.obj", "egg");
    //   object_->FindInstance("egg")->transform.scale = { 0.5f, 0.5f, 0.5f };
    object_->FindInstance("egg")->transform.translate = { 0.0f, 0.5f, 0.0f };

    object_->SetTranslate(pos);

    // サウンド読み込み
    up_ = Audio::GetInstance()->LoadAudio("resources/sounds/up.wav");
    down_ = Audio::GetInstance()->LoadAudio("resources/sounds/down.wav");

    anima_ = std::make_unique<EggAnima>();
    anima_->Initialize(object_.get());
    anima_->Play();

}

void Egg::Finalize() {

}

void Egg::Update() {

    translate = object_->GetTranslate();

#ifdef USE_IMGUI

    ImGui::Begin("Egg");
    ImGui::DragFloat3("pos", &translate.x, 1.0f, 0.0f, 500.0f);
    ImGui::End();

#endif



    if (!gameScene_->IsClear())
    {

        // プレイヤーに持ち上げられていたら
        if (onPlayer_)
        {
            if(!player_->OnThread())
            {
                // スペースキーで卵を置く
                if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
                    // プレイヤーのワールド行列を取得
                    Matrix4x4 worldMatrix = player_->GetWorldMatrix();
                    Vector3 velocity_ = {0.0f, -2.0f, 1.0f};
                    velocity_ = TransformNormal(velocity_, worldMatrix);
                    translate += velocity_;

                    // 置いた先が壁だったら
                    if (CollisionMask::GetInstance()->IsCollisionWall(translate.x, translate.z, kWidth)) {
                        // プレイヤーと同じ場所に置く
                        translate = player_->GetPosition();
                    }

                    onPlayer_ = false;

                    // サウンド再生
                    Audio::GetInstance()->PlayAudio(down_, false, 1.0f);

                    // キャリーアニメーション終了 → リセット処理を含めてIdleに戻す
                    player_->ResetOneShotAnimationAndChangeState(PlayerAnima::AnimationState::Idle);
                    object_->SetTranslate(translate);
                    return;
                }
            }

            // 座標をプレイヤーと同期する
            translate = player_->GetPosition();
            translate.y += 2.0f;

        }
        else
        {
            // プレイヤーと接触していたら
            if (isHit_)
            {
                // スペースキーで卵を持ち上げる
                if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
                {
                    onPlayer_ = true;
                    // サウンド再生
                    Audio::GetInstance()->PlayAudio(up_, false, 1.0f);
                    // キャリーアニメーションを再生
                    player_->ChangeAnimation(PlayerAnima::AnimationState::Carry);
                    translate = player_->GetPosition();
                    translate.y += 2.0f;
                }
            }

        }

    }

    if (isDead_)
    {
        if (a >= 0.0f)
        {
            a -= 0.01f; // 徐々に透明にする
            object_->SetModelInstanceColor("egg", { 1.0f, 1.0f, 1.0f, a });
        }
        else
        {
            SceneManager::GetInstance()->ChangeScene("TitleScene");
        }

        for (auto& effect : explosionEffect_)
        {
            effect->Update();
        }
    }
    else
    {

        // --- 色の演出処理 ---
        flickerCounter_ += 1.0f / 60.0f; // フレーム進捗（60FPS想定）
        Vector4 finalColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 基本は白

        // 1. HP低下による常時明滅 (HP 30%以下などで発動)
        float hpRatio = HP_ / maxHP_;
        if (hpRatio <= 0.3f) {
            // HPが低いほど明滅を速くする (周期を短くする)
            float speed = (hpRatio <= 0.1f) ? 15.0f : 8.0f;
            float sinValue = (sinf(flickerCounter_ * speed) + 1.0f) * 0.5f; // 0.0 ~ 1.0

            // 赤っぽく明滅させる
            finalColor.y = 0.2f + (sinValue * 0.8f);
            finalColor.z = 0.2f + (sinValue * 0.8f);
        }
        if (damageEffectTimer_ > 0.0f) {
            damageEffectTimer_ -= 1.0f / 60.0f;

            // sin波を使って 0.0 ～ 1.0 の値を計算 (15.0fは遷移スピード)
            float t = (sinf(damageEffectTimer_ * 15.0f) + 1.0f) * 0.5f;

            // 赤 {1,0,0,1} とデフォルト色を線形補間(Lerp)する
            finalColor.x = 1.0f; // R成分を 0.0(黒) ～ 1.0(赤) で遷移
            finalColor.y = Lerp(1.0f, 0.0f, t); // G成分を 0.0(黒) ～ 0.0(赤) で遷移
            finalColor.z = Lerp(1.0f, 0.0f, t); // B成分を 0.0(黒) ～ 0.0(赤) で遷移

            finalColor.w = 1.0f;
        }
        else {
            isDamage = false;
        }

        // 最終的な色を適用
        object_->SetModelInstanceColor("egg", finalColor);
    }
    

    object_->SetTranslate(translate);

    anima_->Update();
    object_->Update();

}

void Egg::Draw() {
    object_->Draw();

    if (isDead_)
    {
        for (auto& effect : explosionEffect_)
        {
            effect->Draw();
        }
    }
}

Vector3 Egg::GetWorldPosition() const {
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos = object_->GetTranslate();
    return worldPos;
}

AABB Egg::GetAABB() const {
    Vector3 worldPos = GetWorldPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}

void Egg::OnCollision(const Player* player_) {
    (void)player_;
    isHit_ = true;
}

void Egg::SetHP(float hp)
{
    HP_ -= hp;
    if (hp > 0.0f) {
        isDamage = true;
        damageEffectTimer_ = kDamageEffectTime; // 常に最新のダメージでタイマーをリセット
    }

}
void Egg::Death() {

    if (isDead_)
    {
        return;
    }

    // HPがなくなったら
    if (HP_ <= 0.0f)
    {
        isDead_ = true;
        a = object_->GetColor().w;
        object_->SetBlendMode(BlendMode::Add);

        // エフェクトの生成
        for (int i = 0; i < 10; i++)
        {
            auto effect = std::make_unique<EggExplosion>();
            effect->Initialize(translate);
            explosionEffect_.push_back(std::move(effect));
        }

    }
}