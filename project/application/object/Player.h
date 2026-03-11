#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"

class ThreadManager;

class Player {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="pos">初期位置</param>
  void Initialize(const Vector3 &pos);
  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

  /// <summary>
  /// 移動処理
  /// </summary>
  void UpdateMove();

public:
  // ----- Getter -----

  // 位置
  Vector3 GetPosition() const { return object_->GetTranslate(); }

  // ----- Setter -----

  // ThreadManagerと連携
  void SetThreadManager(ThreadManager *threadManager) {
    threadManager_ = threadManager;
  }

private:
  std::unique_ptr<Object3d> object_;

  Vector3 velocity_ = {0.1f, 0.0f, 0.1f};

  Vector3 scale_ = {1.0f, 1.0f, 1.0f};
  Vector3 rotate_ = {0.0f, 0.0f, 0.0f};
  Vector3 translate_ = {0.0f, 0.0f, 0.0f};

  ThreadManager *threadManager_ = nullptr;
  float weight_ = 0.02f; // プレイヤーの重さ（たわみ具合。値は調整してください）
  float footRadius_ = 0.5f; // 足元の当たり判定の半径
};
