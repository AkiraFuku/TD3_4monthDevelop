#include "PlayerAnima.h"
#include "MathFunction.h"
#include "RotateFunction.h"
#include "Quanternion.h"
#include <cmath>
#include <algorithm> // 追加

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
}

void PlayerAnima::InitializeAnimations()
{
    // --- 1. アイドルモーション ---
    animationMap_[AnimationState::Idle] = Anima::AnimeMove{ 
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Idle);
            float t = std::sin(anima_->GetTimer() * 2.0f * speed) * 0.5f + 0.5f;
            if (instance.name == "Body") {
                instance.transform.translate.y = Lerp(0.0f, 0.2f, t);
            }
        }
    };

    // --- 2. 歩きモーション ---
    animationMap_[AnimationState::Walk] = Anima::AnimeMove{ 
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Walk);
            float t = std::sin(anima_->GetTimer() * 10.0f * speed);
            if (instance.name == "Arm_R") {
                Quaternion start = MakeRotateAxisAngleQuaternion({1, 0, 0}, -0.5f);
                Quaternion end = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.5f);
                instance.transform.rotate = Slerp(start, end, t * 0.5f + 0.5f);
            }
        }
    };

    // --- 3. キャリーモーション ---
    animationMap_[AnimationState::Carry] = Anima::AnimeMove{ 
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_.at(AnimationState::Carry);
            if (instance.name == "Arm_L") {
                Quaternion carryRotate = MakeRotateAxisAngleQuaternion({1, 0, 0}, 0.3f);
                instance.transform.rotate = carryRotate;
            }
        }
    };
}

void PlayerAnima::Update()
{
    anima_->Update();
}

void PlayerAnima::ChangeAnimation(AnimationState newState)
{
    if (currentState_ == newState) {
        return;
    }
    
    currentState_ = newState;
    
    auto it = animationMap_.find(currentState_);
    if (it != animationMap_.end()) {
        anima_->SetCurrentMove(it->second);
    }
}

void PlayerAnima::SetAnimationSpeed(AnimationState state, float speed)
{
    if (animationSpeeds_.find(state) != animationSpeeds_.end()) {
        animationSpeeds_[state] = (std::max)(0.1f, speed); // 最小値は 0.1f
    }
}