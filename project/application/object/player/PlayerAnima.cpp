#include "PlayerAnima.h"
#include "MathFunction.h"
#include "RotateFunction.h"
#include "Quanternion.h"
#include <cmath>
#include <algorithm>

#include <imgui.h>

// キャッシュ用の定数
constexpr uint32_t PART_BODY = 0;
constexpr uint32_t PART_Right_ARM= 1;
constexpr uint32_t PART_Left_ARM = 2;

// インスタンス名からパーツIDへの高速変換
inline uint32_t GetPartId(const std::string& name)
{
    if (name == "Body") return PART_BODY;
    if (name == "RightArm") return PART_Right_ARM;
    if (name == "LeftArm") return PART_Left_ARM;
    return UINT32_MAX;
}

void PlayerAnima::Initialize(Object3d* targetObject)
{
    anima_ = std::make_unique<Anima>();
    anima_->Initialize(targetObject);

    InitializeAnimations();
    InitializeAnimationSpeeds();
    
    // 現在のアニメーションをキャッシュ
    currentAnimationIt_ = animationMap_.find(AnimationState::Idle);
    ChangeAnimation(AnimationState::Idle);
}

void PlayerAnima::InitializeAnimationSpeeds()
{
    animationSpeeds_[AnimationState::Idle] = 1.0f;
    animationSpeeds_[AnimationState::Walk] = 1.0f;
    animationSpeeds_[AnimationState::Carry] = 0.5f;
    animationSpeeds_[AnimationState::OnThread] = 1.0f;
}

void PlayerAnima::InitializeAnimations()
{
    animationMap_.clear();

    animationMap_[AnimationState::Default] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            instance.transform = {
                {1.0f, 1.0f, 1.0f},
                {0.0f, 0.0f, 0.0f, 1.0f},
                {0.0f, 0.0f, 0.0f}
            };
        }
    };

    // --- 1. アイドルモーション ---
    animationMap_[AnimationState::Idle] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Idle);
            float t = std::sin(anima_->GetTimer() * 2.0f * speed) * 0.5f + 0.5f;
            
            uint32_t partId = GetPartId(instance.name);
            if (partId == PART_BODY || partId == PART_Right_ARM || partId == PART_Left_ARM) {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }
        },
        true, false, false, -1.0f
    };

    // --- 2. 歩きモーション ---
    animationMap_[AnimationState::Walk] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Walk);
            float t = std::sin(anima_->GetTimer() * 10.0f * speed);
            
            if (GetPartId(instance.name) == PART_BODY) {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }
        }
    };

    // --- 3. キャリーモーション & OnThread（共通化） ---
    auto CreateRotationAnimation = [this](AnimationState state, bool isLoop) {
        // ★パフォーマンス最適化: クォータニオン計算のキャッシュ
        Quaternion cachedStart = MakeRotateAxisAngleQuaternion({1, 0, 0}, -0.5f);
        Quaternion cachedEnd = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.5f);

        return Anima::AnimeMove{
            [this, cachedStart, cachedEnd](Object3d::ModelInstance& instance) {
                float speed = animationSpeeds_.at(AnimationState::Carry);
                float t = std::sin(anima_->GetTimer() * 10.0f * speed) * 0.5f + 0.5f;

                if (GetPartId(instance.name) & (PART_Right_ARM | PART_Left_ARM)) {
                    // キャッシュされたクォータニオンを使用
                    instance.transform.rotate = Slerp(cachedStart, cachedEnd, t);
                }
            },
            isLoop, false, false, 2.0f
        };
    };

    animationMap_[AnimationState::Carry] = CreateRotationAnimation(AnimationState::Carry, false);
    animationMap_[AnimationState::OnThread] = CreateRotationAnimation(AnimationState::OnThread, true);
}

void PlayerAnima::Update()
{
#ifdef USE_IMGUI
    ImGui::Begin("Animation State");
    ImGui::Text("Current State: %s", 
        (currentState_ == AnimationState::Idle) ? "Idle" :
        (currentState_ == AnimationState::Walk) ? "Walk" :
        (currentState_ == AnimationState::Carry) ? "Carry" :
        (currentState_ == AnimationState::OnThread) ? "OnThread" : "Default");
     ImGui::Text("Can Change Animation: %s", canChangeAnimation_ ? "Yes" : "No");
     // アニメーション速度の調整

     ImGui::End();

#endif //  USE_IMGUI

    anima_->Update();

    // デフォルト遷移中の場合
    if (isInDefaultTransition_) {
        isInDefaultTransition_ = false;
        currentState_ = pendingState_;
        
        currentAnimationIt_ = animationMap_.find(currentState_);
        if (currentAnimationIt_ != animationMap_.end()) {
            anima_->SetCurrentMove(currentAnimationIt_->second);
            anima_->Play();
            
            if (!currentAnimationIt_->second.isLoop) {
                canChangeAnimation_ = false;
                oneShotFinishState_ = AnimationState::Idle;
            }
        }
        return;
    }
    
    // 一回きりアニメーション終了判定
    if (!anima_->IsAnimationPlaying() && !canChangeAnimation_) {
        ResetOneShotAnimation();
        ChangeAnimation(oneShotFinishState_);
    }
}

void PlayerAnima::ChangeAnimation(AnimationState newState)
{
    if (currentState_ == newState) {
        return;
    }

    auto it = animationMap_.find(newState);
    if (it == animationMap_.end()) {
        return;
    }

    if (it->second.isLoop) {
        canChangeAnimation_ = true;
    }

    if (!canChangeAnimation_) {
        return;
    }

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
    auto it = animationSpeeds_.find(state);
    if (it != animationSpeeds_.end()) {
        it->second = (speed < 0.1f) ? 0.1f : speed;
    }
}