#pragma once

#include "ThreadRenderer.h"
#include "PhysicsNode.h"
#include "Vector3.h"
#include <memory>
#include <vector>

class Camera;
class Object3d;

enum class WebState
{
    None,       // 停止中
    Winding,    // 巻き付け中（線を描画）
    Formed      // 繭が完成（モデルのみ描画 / 線は非表示）
};

class EnemyWebEffect {
public:
    void Initialize();
    void Update(const Vector3& targetPos, Camera* camera);
    void Draw();

    void Start();
    void Stop();

    bool IsActive() const { return isActive_; }

private:
    std::unique_ptr<ThreadRenderer> renderer_;
    std::vector<PhysicsNode> nodes_;

    std::unique_ptr<Object3d> cocoonObject_; // 追加: 繭玉モデル
    float cocoonScale_ = 0.0f;

    float wrapAngle_ = 0.0f;
    float wrapHeight_ = 0.0f;
    float wrapDir_ = 1.0f;
    bool isActive_ = false;

    WebState state_ = WebState::None;
    float lineAlpha_ = 1.0f; // 線の透明度（フェード用）

    int totalGeneratedNodes_ = 0;     // これまでに生成した総ノード数
    const int MAX_TOTAL_NODES = 250;  // 巻き付け完了までの総ノード数
    const int MAX_TRAIL_LENGTH = 60;  // 同時に描画する軌跡の最大長（数値を小さくすると尻尾が短くなります）
};
