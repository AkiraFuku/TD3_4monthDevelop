#define DEFINEMINMAX
#include "ThreadManager.h"

#include "Camera.h"

#include <cassert>
#include <cmath>

/// <summary>
/// 初期化
/// </summary>
/// <param name="maxThreads">最大描画本数</param>
/// <param name="nodesPerThread">1本あたりのノード数</param>
void ThreadManager::Initialize(int maxThreads, int nodesPerThread,
                               Camera *camera) {
  // カメラを受け取る
  assert(camera);
  camera_ = camera;

  // 最大描画本数を設定
  maxThreads_ = maxThreads;
  // 1本当たりのノード数を設定
  nodesPerThread_ = nodesPerThread;

  // メモリの再確保を防ぐため、あらかじめ容量を確保
  physicsList_.reserve(maxThreads_);

  renderer_ = std::make_unique<ThreadRenderer>();
  // 太さや分割数はひとまず固定値（必要なら引数化してください）
  renderer_->Initialize(maxThreads, nodesPerThread, 0.02f, 6);
}

/// <summary>
/// 更新
/// </summary>
void ThreadManager::Update() {
  // 描画用のノード配列を収集
  std::vector<std::vector<PhysicsNode>> allNodes;
  allNodes.reserve(physicsList_.size());

  // 各糸の物理演算を更新
  for (auto &physics : physicsList_) {
    physics->Update();
    allNodes.push_back(physics->GetNodes());
  }

  // 収集した全ノードをRendererに渡し、一気に頂点バッファを書き換える
  renderer_->Update(allNodes, camera_);
}

/// <summary>
/// 描画
/// </summary>
void ThreadManager::Draw() { renderer_->Draw(); }

void ThreadManager::AddThread(const Vector3 &startPos, const Vector3 &endPos) {
  // 最大数を超えていたら追加しない（または一番古いものを消す等）
  if (physicsList_.size() >= maxThreads_) {
    return;
  }

  auto physics = std::make_unique<ThreadPhysics>();
  physics->Initialize(startPos, endPos, nodesPerThread_);

  physicsList_.push_back(std::move(physics));
}

bool ThreadManager::InteractWithPlayer(const Vector3 &playerPos,
                                       float playerWeight, float radius,
                                       float &outY) {
  bool isOnThread = false;
  float closestDist = radius; // 判定半径

  for (auto &physics : physicsList_) {
    const auto &nodes = physics->GetNodes();
    for (size_t i = 0; i < nodes.size() - 1; ++i) {
      Vector3 p1 = nodes[i].currentPos;
      Vector3 p2 = nodes[i + 1].currentPos;

      // XZ平面上での線分(p1-p2)とプレイヤー(playerPos)の距離計算
      Vector2 lineDir = {p2.x - p1.x, p2.z - p1.z};
      Vector2 playerToP1 = {playerPos.x - p1.x, playerPos.z - p1.z};

      float lineLenSq = lineDir.x * lineDir.x + lineDir.y * lineDir.y;
      if (lineLenSq == 0.0f)
        continue;

      // 内分比 t を計算 (0.0 〜 1.0 にクランプ)
      float t =
          (playerToP1.x * lineDir.x + playerToP1.y * lineDir.y) / lineLenSq;
      t = std::max(0.0f, std::min(1.0f, t));

      // 線分上の最近傍点
      Vector2 closestPoint = {p1.x + t * lineDir.x, p1.z + t * lineDir.y};

      // 最近傍点とプレイヤーのXZ距離
      float dx = playerPos.x - closestPoint.x;
      float dz = playerPos.z - closestPoint.y;
      float dist = std::sqrt(dx * dx + dz * dz);

      // 判定半径内であれば「乗っている」とみなす
      if (dist < closestDist) {
        closestDist = dist;
        isOnThread = true;

        // ① 高さを補間して取得
        outY = p1.y * (1.0f - t) + p2.y * t;

        // ② プレイヤーの重み（下向きの力）をノードに分配して適用
        Vector3 force = {0.0f, -playerWeight, 0.0f};
        physics->ApplyForce(i, force * (1.0f - t)); // p1側のノードへ
        physics->ApplyForce(i + 1, force * t);      // p2側のノードへ
      }
    }
  }
  return isOnThread;
}

void ThreadManager::ClearThreads() { physicsList_.clear(); }