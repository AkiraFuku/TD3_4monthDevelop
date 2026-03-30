#pragma once

#include <vector>

#include "PhysicsNode.h"
#include "Vector3.h" 

class ThreadPhysics {
public:
    // ---------------------------------------------------------
    // 初期化・更新
    // ---------------------------------------------------------

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="startPos">Threadの始点</param>
    /// <param name="endPos">Threadの終点</param>
    /// <param name="nodeCount">Threadのノード数</param>
    void Initialize(const Vector3& startPos, const Vector3& endPos, int nodeCount);
    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    // ---------------------------------------------------------
    // アクション
    // ---------------------------------------------------------

    /// <summary>
    /// 指定した範囲内にあるノードに力を加える
    /// </summary>
    /// <param name="pos">力を加える中心座標</param>
    /// <param name="radius">力が及ぶ影響範囲の半径</param>
    /// <param name="force">加える力のベクトル</param>
    void ApplyForce(const Vector3& pos, float radius, const Vector3& force);

    // ---------------------------------------------------------
    // ゲッター・セッター
    // ---------------------------------------------------------

    // ノード数
    const std::vector<PhysicsNode>& GetNodes() const { return nodes_; }

    // 重力
    void SetGravity(const Vector3& gravity) { gravity_ = gravity; }
    // 空気抵抗
    void SetDamping(float damping) { damping_ = damping; }
    // Threadの硬さ
    void SetIterations(int iterations) { iterations_ = iterations; }

private:
    // ---------------------------------------------------------
    // 内部処理
    // ---------------------------------------------------------

    /// <summary>
    /// Verlet積分を用いて各ノードの速度と位置を更新する
    /// </summary>
    void Integrate();        // 位置の更新（Verlet積分）
    /// <summary>
    /// ノード間の距離制約を解決し、糸の長さを一定に保つ
    /// </summary>
    void SolveConstraints(); // 距離の制約解決（糸の長さを保つ）

private:
    // ---------------------------------------------------------
    // 状態・パラメーター
    // ---------------------------------------------------------
    std::vector<PhysicsNode> nodes_; // ノード配列
    float segmentLength_ = 0.0f;     // ノード間の自然長

    int iterations_ = 100;           // 制約解決の反復回数（糸の硬さ）
    float damping_ = 0.99f;          // 減衰率（空気抵抗）
    Vector3 gravity_ = {0.0f, -0.0005f, 0.0f}; // 重力
};