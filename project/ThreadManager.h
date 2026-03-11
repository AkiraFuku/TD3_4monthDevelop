#pragma once

#include "ThreadPhysics.h"
#include "ThreadRenderer.h"
#include <Vector4.h>
#include <memory>
#include <vector>

class Camera;

class ThreadManager {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="maxThreads">最大描画本数</param>
  /// <param name="nodesPerThread">1本あたりのノード数</param>
  void Initialize(int maxThreads, int nodesPerThread, Camera *camera);

  /// <summary>
  /// 更新
  /// </summary>
  void Update();

  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

  // プレイヤーから呼ばれる「糸を発射する」処理
  void AddThread(const Vector3 &startPos, const Vector3 &endPos);

  // 全ての糸を消去する
  void ClearThreads();

private:
  // 複数本の物理演算オブジェクト
  std::vector<std::unique_ptr<ThreadPhysics>> physicsList_;

  // 描画は1つ
  std::unique_ptr<ThreadRenderer> renderer_;

  // カメラ
  Camera *camera_ = nullptr;

  int maxThreads_ = 0;
  int nodesPerThread_ = 0;
};
