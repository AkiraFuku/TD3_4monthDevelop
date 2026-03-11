#include "Player.h"

#include "Input.h"
#include "ModelManager.h"
#include "ThreadManager.h"
#include "imgui.h"

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">初期位置</param>
void Player::Initialize(const Vector3 &pos) {
  // モデルの初期化
  object_ = std::make_unique<Object3d>();
  object_->Initialize();

  // 初期位置設定
  translate_ = pos;
  object_->SetTranslate(translate_);

  // モデルを指定
  ModelManager::GetInstance()->LoadModel("player/player.obj");
  object_->SetModel("player/player.obj");
}
/// <summary>
/// 終了
/// </summary>
void Player::Finalize() {}
/// <summary>
/// 更新
/// </summary>
void Player::Update() {

  // 移動処理
  UpdateMove();

#ifdef USE_IMGUI

  ImGui::Begin("Player Window");

  Vector3 scale = object_->GetScale();
  if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
    object_->SetScale(scale);
  }

  Vector3 rotate = object_->GetRotate();
  if (ImGui::DragFloat3("Rotate", &rotate.x, 0.1f, -360.0f, 360.0f)) {
    object_->SetRotate(rotate);
  }

  Vector3 translate = object_->GetTranslate();
  if (ImGui::DragFloat3("Translate", &translate.x, 0.1f, -100.0f, 100.0f)) {
    object_->SetTranslate(translate);
  }

  ImGui::End();

#endif

  // モデルの更新
  object_->Update();
}
/// <summary>
/// 描画
/// </summary>
void Player::Draw() {
  // モデルの描画
  object_->Draw();
}

/// <summary>
/// 移動処理
/// </summary>
void Player::UpdateMove() {
  // 移動方向を蓄積する変数
  Vector3 moveDirection = {0.0f, 0.0f, 0.0f};

  // キー入力に応じて方向ベクトルを決定
  if (Input::GetInstance()->PushedKeyDown(DIK_D)) {
    moveDirection.x += 1.0f;
  } else if (Input::GetInstance()->PushedKeyDown(DIK_A)) {
    moveDirection.x -= 1.0f;
  }

  if (Input::GetInstance()->PushedKeyDown(DIK_W)) {
    moveDirection.z += 1.0f;
  } else if (Input::GetInstance()->PushedKeyDown(DIK_S)) {
    moveDirection.z -= 1.0f;
  }

  if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
    // ベクトルの長さを計算
    float length = std::sqrtf(moveDirection.x * moveDirection.x +
                              moveDirection.z * moveDirection.z);
    // 正規化
    if (length != 0.0f) {
      moveDirection.x /= length;
      moveDirection.z /= length;
    }

    // 正規化した方向に指定速度を掛けて加算
    translate_.x += moveDirection.x * velocity_.x;
    translate_.z += moveDirection.z * velocity_.z;
  }

  if (threadManager_) {
    float threadY = 0.0f;
    // プレイヤーのXZ座標をもとに判定
    if (threadManager_->InteractWithPlayer(translate_, weight_, footRadius_,
                                           threadY)) {
      // 糸に乗っている場合、Y座標を糸の高さ（＋キャラクターの足元オフセット）に合わせる
      // ※足元の原点がモデルの底面にあると仮定
      translate_.y = threadY;
    } else {
      // 糸に乗っていない場合の処理（落下させるなど）
      // translate_.y -= 0.1f; // 簡易的な重力
    }
  }

  object_->SetTranslate(translate_);
}
