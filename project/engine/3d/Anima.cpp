#include "Anima.h"

void Anima::Initialize(Object3d* targetObject)
{
    target_ = targetObject;
    // objが持っているモデルインスタンスをモデルインスタンスリストに登録しておく
    for (const auto& instance : target_->GetModelInstances()) {
        RegisterModelInstance(instance->name, instance.get());
    }
}

void Anima::RegisterModelInstance(const std::string& name, Object3d::ModelInstance* instance)
{
    // ここでは単純にリストに追加するだけですが、名前で検索できるようにするなどの工夫も可能です
    modelInstances_.emplace_back(std::make_unique<Object3d::ModelInstance>(*instance));
}

void Anima::Update()
{
    if (!isPlaying_) return;
    // タイマーを進める
    timer_ += 0.016f; // 仮に1フレームあたり約16ms進める
    // ここでアニメーションのロジックを実装します
    // 例えば、特定のインスタンスを回転させるなど
    for (auto& instance : modelInstances_) {
        if (currentMove_.moveFunction) {
            currentMove_.moveFunction(*instance);
        }
    }
    // アニメーションの状態をターゲットオブジェクトに反映させる
    for (const auto& instance : modelInstances_) {
        Object3d::ModelInstance* targetInstance = target_->FindInstance(instance->name);
        if (targetInstance) {
            targetInstance->transform = instance->transform;
        }
    }
}