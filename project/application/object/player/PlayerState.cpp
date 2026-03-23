#include "PlayerState.h"

#include "Input.h"
#include "Player.h"

#include <memory>

// ======================================
// 待機状態
// ======================================

// 初期化
void PlayerStateIdle::Initialize(Player* player)
{
}

// 更新
void PlayerStateIdle::Update(Player* player)
{
    // WASDで移動
    if (Input::GetInstance()->PushedKeyDown(DIK_W) || Input::GetInstance()->PushedKeyDown(DIK_S) ||
        Input::GetInstance()->PushedKeyDown(DIK_A) || Input::GetInstance()->PushedKeyDown(DIK_D)) {
        player->ChangeState(std::make_unique<PlayerStateMove>());

        return;
    }

    // SPACEで糸を発射
    if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) && player->CanFireThread()) {
        player->ChangeState(std::make_unique<PlayerStateShoot>());
        return;
    }
}

// ======================================
// 移動状態
// ======================================

// 初期化
void PlayerStateMove::Initialize(Player* player)
{
}

// 更新
void PlayerStateMove::Update(Player* player) {
    Vector3 moveDirection = {0.0f, 0.0f, 0.0f};

    // 1. 【脳の処理】キー入力から進みたい方向（意志）を決定する
    if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.x += 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.x += 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.x -= 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.x -= 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D)) {
        moveDirection.x += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A)) {
        moveDirection.x -= 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.z += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.z -= 1.0f;
    }

    // 2. 状態遷移の判断（入力がなければIdleへ）
    if (moveDirection.x == 0.0f && moveDirection.z == 0.0f) {
        player->ChangeState(std::make_unique<PlayerStateIdle>());
        return;
    }

    // 3. ベクトルの正規化（方向を綺麗に整える）
    float length = std::sqrtf(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
    if (length > 0.0f) {
        moveDirection.x /= length;
        moveDirection.z /= length;
    }

    // 4. 【肉体への指示】計算した方向ベクトルを渡して移動してもらう
    player->Move(moveDirection);

    // 5. 状態遷移の判断（SPACEでShootへ）
    if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) && player->CanFireThread()) {
        player->ChangeState(std::make_unique<PlayerStateShoot>());
        return;
    }
}

// ======================================
// 発射状態
// ======================================

// 初期化
void PlayerStateShoot::Initialize(Player* player)
{
    frameCount_ = 0;

    // 状態に入った瞬間に一度だけ糸を発射
    player->FireThread();
}

// 更新
void PlayerStateShoot::Update(Player* player)
{
    // フレームカウントを加算
    frameCount_ += kDeltaTime;

    // 0.5秒で待機状態に遷移
    if (frameCount_ >= 0.5f) {
        player->ChangeState(std::make_unique<PlayerStateIdle>());
    }
}