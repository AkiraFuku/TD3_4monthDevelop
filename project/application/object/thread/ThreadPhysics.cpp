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

    // アニメーションの初期設定
    startPos_ = startPos;
    endPos_ = endPos;
    isAnimating_ = true;
    animTimer_ = 0.0f;

    // ノードを一旦すべて始点（startPos）付近にまとめて配置
    for (int i = 0; i < nodeCount; ++i) {
        PhysicsNode node;
        node.currentPos = startPos;
        node.previousPos = startPos;

        // 両端は固定、それ以外は動く
        node.isFixed = (i == 0 || i == nodeCount - 1);

        nodes_.push_back(node);
    }
}

/// <summary>
/// 更新
/// </summary>
void ThreadPhysics::Update() {
    // アニメーション再生中の場合は専用の更新処理を行い、物理演算はスキップする
    if (isAnimating_) {
        UpdateSpawnAnimation();
        return;
    }

    // --- 通常の物理演算 ---
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
        if (node.isFixed) continue;

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

void ThreadPhysics::AddPositionOffset(int nodeIndex, const Vector3& offset) {
    if (nodeIndex >= 0 && nodeIndex < nodes_.size()) {
        // 現在の位置を直接ずらすことで、Verlet積分の制約として機能させる
        nodes_[nodeIndex].currentPos += offset;
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
        if (node.isFixed) continue;

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

    // 現在のSlack（たわみ率）を適用した目標の長さを計算
    float targetLength = segmentLength_ * slack_;

    for (int i = 0; i < iterations_; ++i) {
        for (size_t j = 0; j < nodes_.size() - 1; ++j) {
            PhysicsNode& n1 = nodes_[j];
            PhysicsNode& n2 = nodes_[j + 1];

            // 両方固定なら何もしない
            if (n1.isFixed && n2.isFixed) continue;

            Vector3 diff = n1.currentPos - n2.currentPos;
            float currentDist = Length(diff);

            // ゼロ除算防止と、すでに距離が正しい場合のスキップ
            if (currentDist <= 1e-6f) continue;

            // 本来あるべき距離との差分（押し引きする割合）
            float fraction = (targetLength - currentDist) / currentDist;

            // ここで Stiffness（剛性）を掛けて力の伝わり具合を調整する
            Vector3 push = diff * fraction * stiffness_;

            // 固定されているノードは動かさない ▼
            if (n1.isFixed) {
                // n1が固定なら、n2だけを100%動かす
                n2.currentPos -= push;
            } else if (n2.isFixed) {
                // n2が固定なら、n1だけを100%動かす
                n1.currentPos += push;
            } else {
                // 両方動くなら、今まで通り半分ずつ動かす
                n1.currentPos += push * 0.5f;
                n2.currentPos -= push * 0.5f;
            }
        }
    }
}

// 新しく追加する関数
void ThreadPhysics::UpdateSpawnAnimation() {
    animTimer_ += 1.0f;

    // 進行度 (0.0 ~ 1.0)
    float t = std::clamp(animTimer_ / animDuration_, 0.0f, 1.0f);

    // イージング (easeOutQuad: 最初は速く、終点付近でゆっくり伸びる)
    float easeT = 1.0f - (1.0f - t) * (1.0f - t);

    Vector3 difference = endPos_ - startPos_;
    Vector3 dir = difference;
    float totalLen = Length(dir);
    if (totalLen > 0.0001f) {
        dir.x /= totalLen; dir.y /= totalLen; dir.z /= totalLen;
    }

    // 蛇行させるための垂直ベクトル（XZ平面に揺らす）を作成
    Vector3 right = {-dir.z, 0.0f, dir.x};
    float rLen = std::sqrt(right.x * right.x + right.z * right.z);
    if (rLen > 0.0001f) {
        right.x /= rLen;
        right.z /= rLen;
    } else {
        right = {1.0f, 0.0f, 0.0f}; // 真下/真上を向いている場合の例外処理
    }

    for (size_t i = 0; i < nodes_.size(); ++i) {
        // このノードの本来の配置割合 (0.0 ~ 1.0)
        float nodeT = static_cast<float>(i) / (nodes_.size() - 1);

        // 全体をeaseTの長さに縮めて配置（レンダラーの計算エラーを防ぐために最低0.001の長さを確保）
        float currentT = nodeT * std::max(easeT, 0.001f);
        Vector3 basePos = startPos_ + (difference * currentT);

        // 波打たせるオフセット計算
        float amplitude = 0.0f;

        // 始点は壁にくっついたままにするため、nodeTが0付近で振幅を0にフェード
        float startFade = std::clamp(nodeT * 10.0f, 0.0f, 1.0f);

        // サイン波で蛇行の動きを作る（25.0f=波の細かさ, 0.8f=波の流れる速度）
        float wave = std::sin(nodeT * 25.0f - animTimer_ * 0.8f);

        // 終点に近づく(tが1に近づく)につれて全体の振幅をゼロにして真っ直ぐにする
        amplitude = 0.4f * (1.0f - t) * wave * startFade;

        // ノードの位置を更新（物理演算中ではないので previousPos も同じにする）
        nodes_[i].currentPos = basePos + (right * amplitude);
        nodes_[i].previousPos = nodes_[i].currentPos;
    }

    // アニメーション終了判定
    if (animTimer_ >= animDuration_) {
        isAnimating_ = false;
    }
}