#include "ThreadManager.h"

#include "Camera.h"

#include <cassert>

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

void ThreadManager::ClearThreads() { physicsList_.clear(); }
