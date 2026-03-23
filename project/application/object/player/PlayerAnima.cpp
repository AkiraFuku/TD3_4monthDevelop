#include "PlayerAnima.h"
#include "MathFunction.h"
#include "RotateFunction.h"
#include "Quanternion.h"
#include <cmath>
#include <algorithm> // 追加

#include <imgui.h>

void PlayerAnima::Initialize(Object3d* targetObject)
{
    anima_ = std::make_unique<Anima>();
    anima_->Initialize(targetObject);

    InitializeAnimations();
    InitializeAnimationSpeeds();
    ChangeAnimation(AnimationState::Idle);
}

void PlayerAnima::InitializeAnimationSpeeds()
{
    animationSpeeds_[AnimationState::Idle] = 1.0f;
    animationSpeeds_[AnimationState::Walk] = 1.0f;
    animationSpeeds_[AnimationState::Carry] = 1.0f;
    animationSpeeds_[AnimationState::OnThread] = 1.0f;
}

void PlayerAnima::InitializeAnimations()
{
    animationMap_.clear();


    animationMap_[AnimationState::Default] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            instance.transform = {
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},  // 単位クォータニオン (x, y, z, w)
        {0.0f, 0.0f, 0.0f}
            };
            // デフォルトのアニメーションは何もしない
        }
    };


    // --- 1. アイドルモーション ---
    animationMap_[AnimationState::Idle] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Idle);
            float t = std::sin(anima_->GetTimer() * 2.0f * speed) * 0.5f + 0.5f;
            if (instance.name == "Body") {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }
            if (instance.name == "Arm_R") {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }
            if (instance.name == "Arm_L") {
               instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }

        },
        true,    // isLoop
        false,   // sTransitioning
        false,   // isFinished
        -1.0f    // duration（無制限）
    };

    // --- 2. 歩きモーション ---
    animationMap_[AnimationState::Walk] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Walk);
            float t = std::sin(anima_->GetTimer() * 10.0f * speed);
             if (instance.name == "Body") {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }

             /*  if (instance.name == "Arm_R") {
                   Quaternion start = MakeRotateAxisAngleQuaternion({1, 0, 0}, -0.5f);
                   Quaternion end = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.5f);
                   instance.transform.rotate = Slerp(start, end, t * 0.5f + 0.5f);
               }*/
           }
    };

    // --- 3. キャリーモーション ---
    animationMap_[AnimationState::Carry] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Carry);
                float t =anima_->GetTimer() * 10.0f * speed;
           if (instance.name == "Body") {
                   Quaternion start = MakeRotateAxisAngleQuaternion({1, 0, 0}, -0.5f);
                   Quaternion end = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.5f);
                   instance.transform.rotate = Slerp(start, end, t * 0.5f + 0.5f);
            }
        },
        false,   // isLoop（一回きり）
        false,   // sTransitioning （遷移中フラグは外部で管理するため、ここではfalse）
        false,   // isFinished （アニメーション終了フラグも外部で管理するため、ここではfalse）
        2.0f     // duration（2秒で終了）
    };
    animationMap_[AnimationState::OnThread] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::OnThread);
                float t = std::sin(anima_->GetTimer() * 10.0f * speed);
           if (instance.name == "Body") {
                   Quaternion start = MakeRotateAxisAngleQuaternion({1, 0, 0}, -0.5f);
                   Quaternion end = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.5f);
                   instance.transform.rotate = Slerp(start, end, t * 0.5f + 0.5f);
            }
        },
        true,   // isLoop（一回きり）
        false,   // sTransitioning
        false,   // isFinished
        2.0f     // duration（2秒で終了）
    };


}

void PlayerAnima::Update()
{

#ifdef  USE_IMGUI

    // ImGuiでアニメーションの状態を表示
    ImGui::Begin("Animation State");
    ImGui::Text("Current State: %s", 
        (currentState_ == AnimationState::Idle) ? "Idle" :
        (currentState_ == AnimationState::Walk) ? "Walk" :
        (currentState_ == AnimationState::Carry) ? "Carry" :
        (currentState_ == AnimationState::OnThread) ? "OnThread" : "Default");
     ImGui::Text("Can Change Animation: %s", canChangeAnimation_ ? "Yes" : "No");
     // アニメーション速度の調整


#endif //  USE_IMGUI

    anima_->Update();

    // デフォルト遷移中の場合、1フレーム待ってから次の状態に遷移
    if (isInDefaultTransition_) {
        isInDefaultTransition_ = false;
        currentState_ = pendingState_;
        
        auto it = animationMap_.find(currentState_);
        if (it != animationMap_.end()) {
            anima_->SetCurrentMove(it->second);
            anima_->Play();
        }
        
        // 遷移後のアニメーションがループ以外なら、切り替え不可にする
        if (it != animationMap_.end() && !it->second.isLoop) {
            canChangeAnimation_ = false;
            // 一回きりアニメーション終了後、Idleに遷移するように設定
            oneShotFinishState_ = AnimationState::Idle;
        }
        return;
    }
    
    // 一回きりアニメーションが終了したら、タイマーと終了フラグをリセット
    if (!anima_->IsAnimationPlaying() && !canChangeAnimation_) {
        // 終了フラグとタイマーをリセット（再度再生可能にする）
        ResetOneShotAnimation();
        
        // 自動的にoneShotFinishState_に遷移
        ChangeAnimation(oneShotFinishState_);
    }
}

void PlayerAnima::ChangeAnimation(AnimationState newState)
{
    if (currentState_ == newState) {
        return;
    }

    // 新しいアニメーション情報を取得
    auto it = animationMap_.find(newState);
    if (it == animationMap_.end()) {
        return;
    }

    const Anima::AnimeMove& newAnimation = it->second;

    // ループアニメーションは常に切り替え可能
    if (newAnimation.isLoop) {
        canChangeAnimation_ = true;
    }

    // 一回きりアニメーション再生中は切り替え禁止
    if (!canChangeAnimation_) {
        return;
    }

    // デフォルト状態を経由して遷移
    pendingState_ = newState;
    isInDefaultTransition_ = true;
    currentState_ = AnimationState::Default;

    auto defaultIt = animationMap_.find(AnimationState::Default);
    if (defaultIt != animationMap_.end()) {
        anima_->SetCurrentMove(defaultIt->second);
        anima_->Play();
    }
}

void PlayerAnima::SetAnimationSpeed(AnimationState state, float speed)
{
    if (animationSpeeds_.find(state) != animationSpeeds_.end()) {
        animationSpeeds_[state] = (std::max)(0.1f, speed); // 最小値は 0.1f
    }
}