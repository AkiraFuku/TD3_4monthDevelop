#pragma once

#include <vector>

#include "PhysicsNode.h"
// ※ Vector3.h などをインクルードする（元コードのVector4.hから実態に合わせて修正）
#include "Vector3.h"

class ThreadPhysics {
public:
    /// <summary>
    /// 糸の初期化
    /// </summary>
    /// <param name="startPos">糸の始点</param>
    /// <param name="endPos">糸の終点</param>
    /// <param name="nodeCount">節の数</param>
    void Initialize(const Vector3& startPos, const Vector3& endPos, int nodeCount);

    /// <summary>
    /// 物理シミュレーションの更新
    /// </summary>
    void Update();

    /// <summary>
    /// 指定範囲内のノードに力を加える（Verlet積分における位置へのインパルス付与）
    /// </summary>
    void ApplyForce(const Vector3& pos, float radius, const Vector3& force);

    /// <summary>
    /// 指定した座標に一番近いノードを固定（ピン留め）してたるみを無くす
    /// </summary>
    void PinNodeNearPosition(const Vector3& targetPos);

public:
    // ======================================
    // Getter / Setter
    // ======================================
    const std::vector<PhysicsNode>& GetNodes() const { return nodes_; }
    Vector3 GetStartPos() const { return nodes_.front().currentPos; }
    Vector3 GetEndPos() const { return nodes_.back().currentPos; }

    void SetGravity(const Vector3& gravity) { gravity_ = gravity; }
    void SetDamping(float damping) { damping_ = damping; }
    void SetIterations(int iterations) { iterations_ = iterations; }

private:
    // ======================================
    // ヘルパーメソッド
    // ======================================
    void Integrate();
    void SolveConstraints();

    // ピン留めアニメーションの目標値を計算してセットアップする
    void SetupPinAnimationTarget(const Vector3& startPos, const Vector3& endPos, const Vector3& targetPos, int pinIndex, float len1, float len2);

    // イージング関数（EaseOutCubic）
    static float EaseOutCubic(float t);

private:
    std::vector<PhysicsNode> nodes_;           // 糸を構成する節の配列

    // シミュレーションの調整パラメーター
    int iterations_ = 100;                     // 制約を解く回数（多いほど硬く伸びない糸になる）
    float damping_ = 0.99f;                    // 空気抵抗（1.0で抵抗なし）
    Vector3 gravity_ = {0.0f, -0.0005f, 0.0f}; // 重力

    // アニメーション用
    bool isPinAnimating_ = false;              // ピン留めアニメーション中か
    float pinProgress_ = 0.0f;                 // アニメーションの進行度 (0.0 ～ 1.0)
    float pinAnimSpeed_ = 0.05f;               // 張るスピード
    int pinIndex_ = -1;                        // 固定する交差点のインデックス

    std::vector<Vector3> startPositions_;      // アニメーション開始時の各ノード座標
    std::vector<Vector3> targetPositions_;     // アニメーション完了時の目標ノード座標
    std::vector<float> segmentLengths_;        // 各節と節の間の現在の理想的な距離
    std::vector<float> targetSegmentLengths_;  // アニメーション完了後の個別の自然長
};