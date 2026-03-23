#include "Anima.h"

void Anima::Initialize(Object3d* targetObject)
{
    target_ = targetObject;
    // インスタンスのリストは参照として保持（コピーではなく）
}

void Anima::Update()
{
    if (!isPlaying_ || !target_) return;

    // タイマーを進める
    timer_ += 0.016f; // 1フレームあたり約16ms

    // ★変更: targetObject の実インスタンスに直接アニメーション適用
    const auto& instances = target_->GetModelInstances();
    for (const auto& instance : instances) {
        if (currentMove_.moveFunction) {
            // 元のインスタンスに対して直接適用
            currentMove_.moveFunction(*instance);
        }
    }
}