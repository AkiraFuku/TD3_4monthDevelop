#include "Player.h"
#include "Input.h"
#include "ModelManager.h"
#include "imgui.h"
#include "CollisionMask.h"

#include"ThreadManager.h"

#include <cmath>
#include <numbers>

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">初期位置</param>
/// <param name="threadManager">ThreadManagerのポインタ</param>
void Player::Initialize(const Vector3& pos, ThreadManager* thread) {

    //ThreadManagerを借りる
    thread_ = thread;

    // モデルの初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
     translate_ = pos;
    object_->SetTranslate(translate_);
    ModelManager::GetInstance()->LoadModel("resources","player/player.obj");
    object_->SetModel("player/player.obj");

    // 待機状態で初期化
    ChangeState(std::make_unique<PlayerStateIdle>());
}
/// <summary>
/// �I��
/// </summary>
void Player::Finalize() {}
/// <summary>
/// �X�V
/// </summary>
void Player::Update() {

    moveVel_ = { 0.0f, 0.0f, 0.0f };    

    if (state_) {
        state_->Update(this);
    }

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

    // �Փ˔���
    IsCollision();

    // �ړ������m��
    ResultMove();

    // ���f���̍X�V
    object_->Update();
}
/// <summary>
/// �`��
/// </summary>
void Player::Draw() {
    // ���f���̕`��
    object_->Draw();
}

/// <summary>
/// 移動処理
/// </summary>
void Player::UpdateMove(Vector3& moveDirection) {
    // 移動方向を蓄積する変数
    moveDirection = {0.0f, 0.0f, 0.0f};

    // キー入力に応じて方向ベクトルを決定
    if (Input::GetInstance()->PushedKeyDown(DIK_D)) {
        moveDirection.x += 1.0f;
    }
    else if (Input::GetInstance()->PushedKeyDown(DIK_A)) {
        moveDirection.x -= 1.0f;
    }
    else
    {
        moveDirection.x = 0.0f;
    }

    if (Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.z += 1.0f;
    }
    else if (Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.z -= 1.0f;
    }
    else
    {
        moveDirection.z = 0.0f;
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

        // 入力方向から角度を計算
        float targetAngleY = std::atan2(moveDirection.x, moveDirection.z);

        // 目標角度と現在の角度の差分
        float diffrence = targetAngleY - rotationY_;

        // 差分を-π ~ πの範囲に収める(最短経路で旋回)
        while (diffrence > std::numbers::pi_v<float>) {
            diffrence -= 2.0f * std::numbers::pi_v<float>;
        }
        while (diffrence < -std::numbers::pi_v<float>) {
            diffrence += 2.0f * std::numbers::pi_v<float>;
        }

        // 現在の角度に差分の一部を加算
        rotationY_ += diffrence * kTurnSpeed;

        // モデルに適用
        rotate_ = {0.0f, rotationY_, 0.0f};
        object_->SetRotate(rotate_);

        // 正規化した方向に指定速度を掛けて加算
        moveVel_.x += moveDirection.x * velocity_.x;
        moveVel_.z += moveDirection.z * velocity_.z;
    }


}

void Player::IsCollision()
{
 
    float nextX = translate_.x + moveVel_.x;
    float nextZ = translate_.z + moveVel_.z;

    // 移動先が壁かどうかチェック
    if (CollisionMask::GetInstance()->IsCollisionWall(nextX, translate_.z, kWidth)) 
    {
        // 壁なら移動をキャンセル（または押し戻し）
        moveVel_.x = 0.0f;
    }

    if (CollisionMask::GetInstance()->IsCollisionWall(translate_.x, nextZ, kWidth))
    {
        // 壁なら移動をキャンセル（または押し戻し）
        moveVel_.z = 0.0f;
    }

    /*float nextX = translate_.x + moveVel_.x;
    if (CollisionMask::GetInstance()->IsWall(nextX, translate_.z))
    {
        moveVel_.x = 0.0f;
    }

    float nextZ = translate_.z + moveVel_.z;
    if (CollisionMask::GetInstance()->IsWall(translate_.x, nextZ))
    {
        moveVel_.z = 0.0f;
    }*/
}

void Player::ResultMove()
{
    translate_ += moveVel_;
    object_->SetTranslate(translate_);
}

/// <summary>
/// 状態遷移
/// </summary>
/// <param name="newState">次の状態</param>
void Player::ChangeState(std::unique_ptr<IPlayerState> newState)
{
    state_ = std::move(newState);
    state_->Initialize(this);
}

/// <summary>
/// 糸を発射する処理
/// </summary>
void Player::FireThread() {
    if (!thread_) {
        return;
    }

    // 向いている方向
    Vector3 forward = GetForward();
    // プレイヤーの位置
    Vector3 playerPos = GetPosition();

    // 始点（プレイヤーの中心より少し上から出すなど、調整可能）
    Vector3 startPos = {playerPos.x, playerPos.y, playerPos.z};

    // 終点（向いている方向に距離を掛けた位置）
    float threadLength = 10.0f; // 糸を飛ばす距離を指定
    Vector3 endPos = {
        startPos.x + forward.x * threadLength,
        startPos.y + forward.y * threadLength,
        startPos.z + forward.z * threadLength
    };

    // ThreadManagerに糸の生成を依頼
    thread_->AddThread(startPos, endPos);
}

void Player::SetPosition(const Vector3& pos)
{
    translate_ = pos;
    object_->SetTranslate(translate_);
}

// 向いている方向
Vector3 Player::GetForward() const
{
    return {std::sin(rotationY_), 0.0f, std::cos(rotationY_)};
}

AABB Player::GetAABB() const
{
    Vector3 worldPos = GetPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}

Matrix4x4 Player::GetWorldMatrix() const
{
    Matrix4x4 worldMatrix = MakeAfineMatrix(scale_, rotate_, translate_);
    return worldMatrix;
}