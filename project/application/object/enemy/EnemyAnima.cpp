#include "EnemyAnima.h"
#include "MathFunction.h"
#include "RotateFunction.h"
#include "Quanternion.h"
#include <cmath>

void EnemyAnima::Initialize(Object3d* targetObject)
{
    anima_ = std::make_unique<Anima>();
    anima_->Initialize(targetObject);

    InitializeAnimationSpeeds();
    InitializeAnimations();

    ChangeAnimation(AnimationState::Idle);
}

void EnemyAnima::InitializeAnimationSpeeds()
{
    animationSpeeds_[AnimationState::Idle] = 1.0f;
    animationSpeeds_[AnimationState::Walk] = 2.0f;   // 移動は少し速め
    animationSpeeds_[AnimationState::Attack] = 1.5f; // 攻撃のキレ
}

void EnemyAnima::InitializeAnimations()
{
    animationMap_.clear();

    // --- 0. Default (リセット用) ---
    animationMap_[AnimationState::Default] = Anima::AnimeMove{
        [](Object3d::ModelInstance& instance) {
            instance.transform.translate = {0,0,0};
            instance.transform.rotate = {0,0,0,1};
        }
    };

    // --- 1. Idle (ゆっくり浮遊) ---
    animationMap_[AnimationState::Idle] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_[AnimationState::Idle];
            float t = std::sin(anima_->GetTimer() * 2.0f * speed) * 0.5f + 0.5f;
            instance.transform.translate.y = Lerp(-0.1f, 0.1f, t);
        },
        true // ループ
    };

    // --- 2. Walk (激しく上下に跳ねる) ---
    animationMap_[AnimationState::Walk] = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float speed = animationSpeeds_[AnimationState::Walk];
            // 絶対値を取ることで「ぴょんぴょん」跳ねる動きにする
            float t = std::abs(std::sin(anima_->GetTimer() * 5.0f * speed));
            instance.transform.translate.y = Lerp(0.0f, 0.5f, t);
        },
        true
    };

    // --- 3. Attack (前方に鋭く突き出す) ---
// --- 3. Attack (大きく振りかぶってから振り下ろす) ---
  animationMap_[AnimationState::Attack] = Anima::AnimeMove{
    [this](Object3d::ModelInstance& instance) {
        float speed = animationSpeeds_[AnimationState::Attack];
        
        // 1. 傾きの範囲を定義 (X軸を中心に -0.5ラジアン 〜 0.5ラジアン)
        // 数値を変えることで傾きの深さを調整できます
        Quaternion rotateStart = MakeRotateAxisAngleQuaternion({1.0f, 0.0f, 0.0f}, -0.5f);
        Quaternion rotateEnd   = MakeRotateAxisAngleQuaternion({1.0f, 0.0f, 0.0f}, 0.5f);

        // 2. sin波を使って 0.0 ~ 1.0 の間で揺らす
        // PlayerAnimaのOnThread (10.0f * speed) を参考に周期を設定
        float t = std::sin(anima_->GetTimer() * 5.0f/60.0f * speed) * 0.5f + 0.5f;

        // 3. クォータニオンで補完して回転を適用
        instance.transform.rotate = Slerp(rotateStart, rotateEnd, t);

        // オプション：上下の跳ねも加える場合 (PlayerのWalkに近い表現)
        float jumpT = std::abs(std::sin(anima_->GetTimer() * 10.0f * speed));
        instance.transform.translate.y = Lerp(0.0f, 0.3f, jumpT);
    },
    false, // Attackが終わったらIdleに戻したい場合はfalse
    false, 
    false, 
    1.0f   // 非ループの場合の継続時間
};
}

void EnemyAnima::Update()
{
    anima_->Update();

    // 状態遷移の処理 (PlayerAnimaのロジックを簡略化して継承)
    if (isInDefaultTransition_) {
        isInDefaultTransition_ = false;
        currentState_ = pendingState_;
        auto it = animationMap_.find(currentState_);
        if (it != animationMap_.end()) {
            anima_->SetCurrentMove(it->second);
            anima_->Play();
        }
        return;
    }

    // 単発アニメーション（攻撃など）が終わったらIdleに戻る判定
    if (!animationMap_[currentState_].isLoop && !anima_->IsAnimationPlaying()) {
        ChangeAnimation(AnimationState::Idle);
    }
}

void EnemyAnima::ChangeAnimation(AnimationState newState)
{
    if (currentState_ == newState) return;

    auto it = animationMap_.find(newState);
    if (it == animationMap_.end()) return;

    // 遷移の瞬間に一度Default（初期姿勢）を挟む
    pendingState_ = newState;
    isInDefaultTransition_ = true;
    currentState_ = AnimationState::Default;

    auto defaultIt = animationMap_.find(AnimationState::Default);
    if (defaultIt != animationMap_.end()) {
        anima_->SetCurrentMove(defaultIt->second);
        anima_->Play();
    }
}

void EnemyAnima::SetAnimationSpeed(AnimationState state, float speed)
{
    animationSpeeds_[state] = speed;
}