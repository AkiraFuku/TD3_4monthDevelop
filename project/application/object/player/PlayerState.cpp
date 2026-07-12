
#include "PlayerState.h"

#include "Input.h"
#include "Player.h"
#include "OneWayObject.h"
#include "DXCommon.h"
#include <memory>
#include "Audio.h"
static const float kStickMax = 32767.0f;
static const float kDeadZone = 0.3f;

// ======================================
// 待機状態
// ======================================

// 初期化
void PlayerStateIdle::Initialize(Player* player) {
    player->ChangeAnimation(PlayerAnima::AnimationState::Idle);
}

// 更新
void PlayerStateIdle::Update(Player* player) {
    OneWayObject* oneWay = player->CheckOnOneWayObject();

    // オブジェクトに触れていて、かつ「入り口側」から入ろうとしている場合のみ遷移
    if (oneWay && oneWay->IsAtEntrySide(player->GetPosition())) {
        player->SetCurrentOneWay(oneWay);
        player->ChangeState(std::make_unique<PlayerStateOneWayMove>());
        return;
    }

    bool isMoveInput = false;

    // 1. WASDで移動判定
    if (Input::GetInstance()->PushedKeyDown(DIK_W) || Input::GetInstance()->PushedKeyDown(DIK_S) ||
        Input::GetInstance()->PushedKeyDown(DIK_A) || Input::GetInstance()->PushedKeyDown(DIK_D)) {
        isMoveInput = true;
    }

    // 2. コントローラーで移動判定
    XINPUT_STATE joyState{};
    if (Input::GetInstance()->GetJoyStick(0, joyState)) {
        // スティックの入力を -1.0f ～ 1.0f に正規化
        float stickX = (float)joyState.Gamepad.sThumbLX / kStickMax;
        float stickY = (float)joyState.Gamepad.sThumbLY / kStickMax;

        // デッドゾーンを超えていたら移動とみなす
        if (std::abs(stickX) > kDeadZone || std::abs(stickY) > kDeadZone) {
            isMoveInput = true;
        }
    }

    // 移動入力があれば移動状態へ
    if (isMoveInput) {
        player->ChangeState(std::make_unique<PlayerStateMove>());
        return;
    }

    // SPACE または コントローラーのAボタンで糸を発射
    if ((Input::GetInstance()->TriggerKeyDown(DIK_B) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER))) {

        player->ChangeState(std::make_unique<PlayerStateShoot>());

        return;
    }
}

// ======================================
// 移動状態
// ======================================

// 初期化
void PlayerStateMove::Initialize(Player* player) {
    if (player->OnThread())
    {
        player->ChangeAnimation(PlayerAnima::AnimationState::OnThread);

    } else {
        player->ChangeAnimation(PlayerAnima::AnimationState::Walk);

    }
}

// 更新
void PlayerStateMove::Update(Player* player) {
    OneWayObject* oneWay = player->CheckOnOneWayObject();
    if (oneWay) {
        player->SetCurrentOneWay(oneWay);
        player->ChangeState(std::make_unique<PlayerStateOneWayMove>());
        return;
    }

    // 歩行音の再生
    // 一秒ごとに歩行音を再生する
    SEWalkTimer_ += DirectXCommon::kDeltaTime;
    if (SEWalkTimer_ >= 0.5f) {
        Audio::GetInstance()->PlayAudio(player->GetWalkSoundHandle(), false, 1.0f);
        SEWalkTimer_ = 0.0f;
    }
    Vector3 moveDirection = { 0.0f, 0.0f, 0.0f };

    // 1. 【脳の処理】進みたい方向（意志）を決定する

    // コントローラー入力を取得
    XINPUT_STATE joyState{};
    bool isPadInput = false;

    if (Input::GetInstance()->GetJoyStick(0, joyState)) {
        float stickX = (float)joyState.Gamepad.sThumbLX / kStickMax;
        float stickY = (float)joyState.Gamepad.sThumbLY / kStickMax;

        if (std::abs(stickX) > kDeadZone || std::abs(stickY) > kDeadZone) {
            // スティックのXを移動のX、Y(上下)を移動のZ(奥手前)に割り当てる
            moveDirection.x = stickX;
            moveDirection.z = stickY;
            isPadInput = true;
        }
    }

    // パッド入力がなければキー入力から進みたい方向を決定する
    if (!isPadInput) {
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

    // ==========================================
    // ★ここから追加・変更
    // ==========================================
    // VキーまたはLBボタンが押されているか（狙いを定めている状態か）判定
    bool isAiming = false;
    if (Input::GetInstance()->PushedKeyDown(DIK_LSHIFT) || Input::GetInstance()->PushPadDown(0, XINPUT_GAMEPAD_LEFT_SHOULDER)) {
        isAiming = true;
    }

    if (isAiming) {
        // 狙いを定めている間は移動せず、入力方向への回転のみ行う
        player->TurnToDirection(moveDirection);

        // 動いていないのでアニメーションを待機状態にする
        player->ChangeAnimation(PlayerAnima::AnimationState::Idle);
    } else {
        // ★重要: 移動前のスレッド状態を記録
        bool wasOnThread = player->OnThread();

        // 4. 【肉体への指示】計算した方向ベクトルを渡して移動してもらう
        player->Move(moveDirection);

        // 5. スレッド上の移動処理
        if (player->OnThread()) {
            // ResolveThreadMove は Player::Update で呼ばれるので、ここでは不要
        } else {
            player->IsCollisionSDF();
        }

        // ★重要: 状態が変わったかを比較
        bool isNowOnThread = player->OnThread();

        if (!wasOnThread && isNowOnThread) {
            // 地面 → スレッド上に乗った
            player->ChangeAnimation(PlayerAnima::AnimationState::OnThread);
        } else if (wasOnThread && !isNowOnThread) {
            // スレッド上 → 地面に降りた
            player->ChangeAnimation(PlayerAnima::AnimationState::Walk);
        } else {
            // 状態が変わっていない場合も、ボタンを離した瞬間に歩行アニメが再開するように設定
            if (player->OnThread()) {
                player->ChangeAnimation(PlayerAnima::AnimationState::OnThread);
            } else {
                player->ChangeAnimation(PlayerAnima::AnimationState::Walk);
            }
        }
    }

    // 6. 状態遷移の判断（B または コントローラーのRBボタン でShootへ）
    if ((Input::GetInstance()->TriggerKeyDown(DIK_B) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER))) {

        player->ChangeState(std::make_unique<PlayerStateShoot>());
        return;
    }
}

// ======================================
// 発射状態
// ======================================

// 初期化
void PlayerStateShoot::Initialize(Player* player) {
    player->SetCanDrawPrediction(false);

    frameCount_ = 0;

    // 状態に入った瞬間に一度だけ糸を発射
    player->FireThread();
}

// 更新
void PlayerStateShoot::Update(Player* player) {
    player->SetCanDrawPrediction(false);

    // フレームカウントを加算
    frameCount_ += kDeltaTime;

    // 0.5秒で待機状態に遷移
    if (frameCount_ >= 0.5f) {
        player->ChangeState(std::make_unique<PlayerStateIdle>());
    }
}

// ======================================
// 一方通行オブジェクト自動移動状態
// ======================================

void PlayerStateOneWayMove::Initialize(Player* player) {
    // 必要に応じて歩行などのアニメーションに変更する
    player->ChangeAnimation(PlayerAnima::AnimationState::Walk);
}

void PlayerStateOneWayMove::Update(Player* player) {
    OneWayObject* oneWay = player->GetCurrentOneWay();


    if (!Audio::GetInstance()->IsPlaying(player->GetWalkSoundHandle()))
    {
        Audio::GetInstance()->PlayAudio(player->GetWalkSoundHandle(), false, 2.0f);
    }

    // OneWayObjectから外れた（終点を超えた）場合は、Idle状態に戻る
    if (!oneWay || !oneWay->IsInside(player->GetPosition())) {
        player->SetCurrentOneWay(nullptr);
        player->ChangeState(std::make_unique<PlayerStateIdle>());
        return;
    }

    // OneWayObjectの許可されている方向を取得して自動移動ベクトルを決定
    Vector3 autoMoveDir = { 0.0f, 0.0f, 0.0f };
    switch (oneWay->GetDirection()) {
    case OneWayObject::Direction::PositiveX: autoMoveDir = { 1.0f, 0.0f, 0.0f }; break;
    case OneWayObject::Direction::NegativeX: autoMoveDir = { -1.0f, 0.0f, 0.0f }; break;
    case OneWayObject::Direction::PositiveZ: autoMoveDir = { 0.0f, 0.0f, 1.0f }; break;
    case OneWayObject::Direction::NegativeZ: autoMoveDir = { 0.0f, 0.0f, -1.0f }; break;
    }

    // WASD入力関係なく、強制的に進行方向へ移動させる
    player->Move(autoMoveDir);

    // 自動移動中も壁などのSDF衝突判定は行う
    //player->IsCollisionSDF();
}