#pragma once
#include "Anima.h"
#include <map>
#include <memory>

class PlayerAnima
{
public:
    // アニメーションの種類を定義
    enum class AnimationState {
        Default = -1,
        Idle,
        Walk,
        Carry,
        OnThread,
        Clear, // ← 追加

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
    
    // 一回きりアニメーション終了後の遷移先を設定
    void SetOneShotFinishState(AnimationState state) {
        oneShotFinishState_ = state;
    }
    
    // 一回きりアニメーション終了後のリセット処理
    void ResetOneShotAnimation() {
        if (currentState_ != AnimationState::Default) {
            auto it = animationMap_.find(currentState_);
            if (it != animationMap_.end() && !it->second.isLoop) {
                // タイマーと終了フラグをリセット
                anima_->ResetAnimation();
                canChangeAnimation_ = true;
            }
        }
    }
    
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

    AnimationState pendingState_ = AnimationState::Idle;   // 次の状態
    bool isInDefaultTransition_ = false;                    // デフォルト遷移中フラグ
    bool canChangeAnimation_ = true;                        // アニメーション変更可能フラグ
    
    // 一回きりアニメーション終了後の遷移先
    AnimationState oneShotFinishState_ = AnimationState::Idle;  // 終了後の遷移先状態

    // アニメーションマップのイテレータ（現在のアニメーションを指す）
    std::map<AnimationState, Anima::AnimeMove>::iterator currentAnimationIt_;
};

