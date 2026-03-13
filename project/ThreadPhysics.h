#pragma once

#include <PhysicsNode.h>
#include <Vector4.h>

class ThreadPhysics {
public:
    /// <summary>
    /// 糸の初期化
    /// </summary>
    /// <param name="startPos">糸の始点</param>
    /// <param name="endPos">糸の終点</param>
    /// <param name="nodeCount">節の数</param>
    void Initialize(const Vector3& startPos, const Vector3& endPos,
        int nodeCount);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    // Getter
    const std::vector<PhysicsNode>& GetNodes() const { return nodes_; }

    // パラメータ調整用セッター（ImGuiなどで調整すると便利です）
    void SetGravity(const Vector3& gravity) { gravity_ = gravity; }
    void SetDamping(float damping) { damping_ = damping; }
    void SetIterations(int iterations) { iterations_ = iterations; }

private:
    std::vector<PhysicsNode> nodes_; // 糸を構成する節の配列
    float segmentLength_ = 0.0f;     // 節と節の間の理想的な距離

    // シミュレーションの調整パラメーター
    int iterations_ = 100;  // 制約を解く回数（多いほど硬く伸びない糸になる）
    float damping_ = 0.99f; // 空気抵抗（1.0で抵抗なし）
    Vector3 gravity_ = { 0.0f, -0.0005f, 0.0f }; // 重力
};
