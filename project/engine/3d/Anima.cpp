#include "Anima.h"

void Anima::Initialize(Object3d* targetObject)
{
    target_ = targetObject;
    // インスタンスのリストは参照として保持（コピーではなく）
}

void Anima::Update()
{
    if (!isPlaying_ || !target_ || isAnimationFinished_) return;

    // タイマーを進める
    timer_ += 0.016f; // 1フレームあたり約16ms

    // アニメーション期間が設定されている場合、終了判定
    if (currentMove_.duration > 0.0f && timer_ >= currentMove_.duration) {
        if (!currentMove_.isLoop) {
            isAnimationFinished_ = true;  // 一回きりの場合、終了フラグを立てる
        } else {
            timer_ = 0.0f;  // ループの場合、タイマーをリセット
        }
    }

    // ★変更: targetObject の実インスタンスに直接アニメーション適用
    const auto& instances = target_->GetModelInstances();
    for (const auto& instance : instances) {
        if (currentMove_.moveFunction) {
            // 元のインスタンスに対して直接適用
            currentMove_.moveFunction(*instance);
        }
    }
}