#pragma once
#include "Anima.h"
#include <map>
#include <memory>
#include <string>

class EnemyAnima
{
public:
    // エネミーの状態定義
    enum class AnimationState {
        Default = -1,
        Idle,
        Walk,
        Attack,
    };

    void Initialize(Object3d* targetObject);
    void Update();
    void ChangeAnimation(AnimationState newState);
    
    void Play() { anima_->Play(); }
    void Stop() { anima_->Stop(); }
    
    void SetAnimationSpeed(AnimationState state, float speed);

private:
    std::unique_ptr<Anima> anima_;
    AnimationState currentState_ = AnimationState::Idle;
    AnimationState pendingState_ = AnimationState::Idle;
    bool isInDefaultTransition_ = false;

    // アニメーションデータと速度の管理
    std::map<AnimationState, Anima::AnimeMove> animationMap_;
    std::map<AnimationState, float> animationSpeeds_;

    void InitializeAnimations();
    void InitializeAnimationSpeeds();
};