#include "ThreadPhysics.h"

#include "MathFunction.h"

/// <summary>
/// 糸の初期化
/// </summary>
/// <param name="startPos">糸の始点</param>
/// <param name="endPos">糸の終点</param>
/// <param name="nodeCount">節の数</param>
void ThreadPhysics::Initialize(const Vector3 &startPos, const Vector3 &endPos,
                               int nodeCount) {
  nodes_.clear();

  // 始点から終点へのベクトル
  Vector3 diff = endPos - startPos;

  // 全体の長さから、1区間（セグメント）の理想的な長さを計算
  float totalLength = Length(diff);
  segmentLength_ = totalLength / static_cast<float>(nodeCount - 1);

  for (int i = 0; i < nodeCount; ++i) {
    // 0.0(始点) ～ 1.0(終点) の割合
    float t = static_cast<float>(i) / static_cast<float>(nodeCount - 1);

    // 現在のノードの初期座標を補間計算
    Vector3 pos = startPos + (diff * t);

    PhysicsNode node;
    node.currentPos = pos;
    node.previousPos = pos;

    // i == 0 (始点) と i == nodeCount - 1 (終点)を質量0にして壁に固定
    // 片方下げは(i == 0) だけを 0.0f にする
    if (i == 0 || i == nodeCount - 1) {
      node.mass = 0.0f; // 固定
    } else {
      node.mass = 1.0f; // 動く
    }

    nodes_.push_back(node);
  }
}

/// <summary>
/// 更新
/// </summary>
void ThreadPhysics::Update() {
  // ==========================================
  // ① Verlet積分（慣性と重力で動かす）
  // ==========================================
  for (auto &node : nodes_) {
    // 質量0（固定ノード）は動かさない
    if (node.mass == 0.0f)
      continue;

    // 速度ベクトル ＝ 現在位置 － 過去位置
    Vector3 velocity = node.currentPos - node.previousPos;

    // 過去位置を現在の位置で更新
    node.previousPos = node.currentPos;

    // 新しい位置 ＝ 現在位置 ＋ (速度 × 空気抵抗) ＋ 重力
    node.currentPos += (velocity * damping_) + gravity_;
  }

  // ==========================================
  // ② 制約の解決（ノード間の長さを保つ）
  // ==========================================
  for (int i = 0; i < iterations_; ++i) {
    for (size_t j = 0; j < nodes_.size() - 1; ++j) {
      PhysicsNode &n1 = nodes_[j];
      PhysicsNode &n2 = nodes_[j + 1];

      // 両方とも固定ノードなら計算をスキップ
      if (n1.mass == 0.0f && n2.mass == 0.0f) {
        continue;
      }

      // 2点間のベクトル
      Vector3 diff = n1.currentPos - n2.currentPos;

      // 2点間の現在の距離
      float currentDist = Length(diff);
      if (currentDist == 0.0f) {
        continue; // ゼロ除算回避
      }

      // 理想の長さとのズレの割合を計算
      float difference = (segmentLength_ - currentDist) / currentDist;

      // 押し引きする移動量（ベクトル × スカラー）
      Vector3 move = diff * (difference * 0.5f);

      // 位置を修正（片方が固定なら、もう片方だけが移動するようにする）
      if (n1.mass > 0.0f) {
        n1.currentPos += move;
      }
      if (n2.mass > 0.0f) {
        n2.currentPos -= move;
      }
    }
  }
}

void ThreadPhysics::ApplyForce(size_t nodeIndex, const Vector3 &force) {
  // 範囲外アクセス防止と、固定ノード（mass == 0）には力を加えない処理
  if (nodeIndex < nodes_.size() && nodes_[nodeIndex].mass > 0.0f) {
    nodes_[nodeIndex].currentPos += force;
  }
}
