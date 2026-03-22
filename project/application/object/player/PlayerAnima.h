#pragma once
#include "Anima.h"
#include <map>
#include <memory>

class PlayerAnima
{
public:
    // アニメーションの種類を定義
    enum class AnimationState {
        Idle,
        Walk,
        Carry,
    };
    
    void Initialize(Object3d* targetObject);
    void Update();
    void ChangeAnimation(AnimationState newState);
    void Play() { anima_->Play(); }
    void Stop() { anima_->Stop(); }
    
    // 現在の状態を取得
    AnimationState GetCurrentState() const { return currentState_; }
    
    // アニメーション再生速度の設定
    void SetAnimationSpeed(AnimationState state, float speed);
    
private:
    std::unique_ptr<Anima> anima_;
    AnimationState currentState_ = AnimationState::Idle;

    // アニメーションの定義を格納するマップ
    std::map<AnimationState, Anima::AnimeMove> animationMap_;
    
    // 再生速度を格納するマップ
    std::map<AnimationState, float> animationSpeeds_;
    
    // アニメーション定義用ヘルパー関数
    void InitializeAnimations();
    void InitializeAnimationSpeeds();
};

