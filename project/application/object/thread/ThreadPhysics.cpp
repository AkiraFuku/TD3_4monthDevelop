#include "ThreadPhysics.h"
#include "MathFunction.h"

#include <cmath>
#include <algorithm>

// ---------------------------------------------------------
// 初期化・更新
// ---------------------------------------------------------

/// <summary>
/// 初期化
/// </summary>
/// <param name="startPos">Threadの始点</param>
/// <param name="endPos">Threadの終点</param>
/// <param name="nodeCount">Threadのノード数</param>
void ThreadPhysics::Initialize(const Vector3& startPos, const Vector3& endPos, int nodeCount) {
    // 最低2つのノードが必要
    if (nodeCount < 2) return;

    // メモリ再確保を防ぐ
    nodes_.clear();
    nodes_.reserve(nodeCount);

    // 全体の長さと1区間の長さを計算
    Vector3 difference = endPos - startPos;
    float totalLength = Length(difference);
    segmentLength_ = totalLength / static_cast<float>(nodeCount - 1);

    // ノードを直線上に配置
    for (int i = 0; i < nodeCount; ++i) {
        float t = static_cast<float>(i) / (nodeCount - 1);
        Vector3 pos = startPos + (difference * t);

        PhysicsNode node;
        node.currentPos = pos;
        node.previousPos = pos;

        // 両端は固定(質量0)、それ以外は動く(質量1)
        node.mass = (i == 0 || i == nodeCount - 1) ? 0.0f : 1.0f;

        nodes_.push_back(node);
    }
}

/// <summary>
/// 更新
/// </summary>
void ThreadPhysics::Update() {
    Integrate();        // 1. 速度と重力の適用
    SolveConstraints(); // 2. 長さの補正
}

// ---------------------------------------------------------
// アクション
// ---------------------------------------------------------

/// <summary>
/// 指定した範囲内にあるノードに力を加える
/// </summary>
/// <param name="pos"></param>
/// <param name="radius"></param>
/// <param name="force"></param>
void ThreadPhysics::ApplyForce(const Vector3& pos, float radius, const Vector3& force) {
    if (radius <= 0.0f) return;

    // 距離判定用の半径二乗
    float radiusSq = radius * radius;

    for (auto& node : nodes_) {
        // 固定ノードは無視
        if (node.mass <= 0.0f) continue;

        Vector3 diff = node.currentPos - pos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        // 影響範囲内か判定
        if (distSq <= radiusSq) {
            float dist = std::sqrt(distSq);

            // 中心に近いほど強く(1.0 -> 0.0)
            float falloff = 1.0f - (dist / radius);

            // 位置のズレとして力を加える（次フレームで速度になる）
            node.currentPos += force * falloff;
        }
    }
}

// ---------------------------------------------------------
// 内部処理
// ---------------------------------------------------------

/// <summary>
/// Verlet積分を用いて各ノードの速度と位置を更新する
/// </summary>
void ThreadPhysics::Integrate() {
    for (auto& node : nodes_) {
        // 固定ノードは無視
        if (node.mass <= 0.0f) continue;

        // 現在の速度を算出（Verlet積分）
        Vector3 velocity = node.currentPos - node.previousPos;

        // 現在位置を過去位置として保存
        node.previousPos = node.currentPos;

        // 慣性 + 重力で次の位置を予測
        node.currentPos += (velocity * damping_) + gravity_;
    }
}

/// <summary>
/// ノード間の距離制約を解決し、糸の長さを一定に保つ
/// </summary>
void ThreadPhysics::SolveConstraints() {
    if (nodes_.size() < 2) return;

    // 指定回数だけ補正を繰り返し、糸を硬くする
    for (int i = 0; i < iterations_; ++i) {
        for (size_t j = 0; j < nodes_.size() - 1; ++j) {
            PhysicsNode& n1 = nodes_[j];
            PhysicsNode& n2 = nodes_[j + 1];

            // 両方固定なら計算不要
            if (n1.mass <= 0.0f && n2.mass <= 0.0f) continue;

            Vector3 diff = n1.currentPos - n2.currentPos;
            float currentDist = Length(diff);

            // ゼロ除算防止
            if (currentDist <= 1e-6f) continue;

            // 理想の長さとの誤差割合
            float difference = (segmentLength_ - currentDist) / currentDist;

            // 動くノードの数をカウント（1 or 2）
            float weightSum = (n1.mass > 0.0f ? 1.0f : 0.0f) + (n2.mass > 0.0f ? 1.0f : 0.0f);

            // 補正ベクトルを計算
            Vector3 correction = diff * (difference / weightSum);

            // 動くノードに対して補正を適用
            if (n1.mass > 0.0f) n1.currentPos += correction;
            if (n2.mass > 0.0f) n2.currentPos -= correction;
        }
    }
}