#include "Egg.h"
#include "ModelManager.h"
#include "imgui.h"
#include "Input.h"
#include "Player.h"
#include "CollisionMask.h"
#include "SceneManager.h"

void Egg::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();


    ModelManager::GetInstance()->LoadModel("resources", "egg.obj");
    object_->AddModel("egg.obj","egg");
 //   object_->FindInstance("egg")->transform.scale = { 0.5f, 0.5f, 0.5f };
    object_->FindInstance("egg")->transform.translate = { 0.0f, 0.5f, 0.0f };

    object_->SetTranslate(pos);
    
    anima_ = std::make_unique<EggAnima>();
    anima_->Initialize(object_.get());
    anima_->Play();



}

void Egg::Finalize() {

}

void Egg::Update() {

    Vector3 translate = object_->GetTranslate();

    // プレイヤーに持ち上げられていたら
    if (onPlayer_)
    {
        // スペースキーで卵を置く
        if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
        {
            // プレイヤーのワールド行列を取得
            Matrix4x4 worldMatrix = player_->GetWorldMatrix();
            Vector3 velocity_ = {0.0f, -2.0f, 1.0f};
            velocity_ = TransformNormal(velocity_, worldMatrix);
            translate += velocity_;

            // 置いた先が壁だったら
            if (CollisionMask::GetInstance()->IsCollisionWall(translate.x, translate.z, kWidth))
            {
                // プレイヤーと同じ場所に置く
                translate = player_->GetPosition();
            }

            onPlayer_ = false;
            // キャリーアニメーション終了 → リセット処理を含めてIdleに戻す
            player_->ResetOneShotAnimationAndChangeState(PlayerAnima::AnimationState::Idle);
            object_->SetTranslate(translate);
            return;
        }

        // 座標をプレイヤーと同期する
        translate = player_->GetPosition();
        translate.y += 2.0f;

    } else
    {
        // プレイヤーと接触していたら
        if (isHit_)
        {
            // スペースキーで卵を持ち上げる
            if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
            {
                onPlayer_ = true;
                // キャリーアニメーションを再生
                player_->ChangeAnimation(PlayerAnima::AnimationState::Carry);
                translate = player_->GetPosition();
                translate.y += 2.0f;
            }
        }
        //else
        //{
        //    // ゴール判定確認用の移動処理
        //    if (Input::GetInstance()->PushedKeyDown(DIK_UP))
        //    {
        //        // 奥に進む
        //        translate.z += 0.1f;
        //    }
        //    else if (Input::GetInstance()->PushedKeyDown(DIK_DOWN))
        //    {
        //        // 手前に進む
        //        translate.z -= 0.1f;
        //    }
        //}
    }

    object_->SetTranslate(translate);

    anima_->Update();
    object_->Update();

}

void Egg::Draw() {
    object_->Draw();
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

    aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
    aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

    return aabb;
}

void Egg::OnCollision(const Player* player_) {
    (void) player_;
    isHit_ = true;
}

void Egg::Death() {
    // HPがなくなったら
    if (HP_ <= 0.0f)
    {
        SceneManager::GetInstance()->ChangeScene("TitleScene");
    }
}