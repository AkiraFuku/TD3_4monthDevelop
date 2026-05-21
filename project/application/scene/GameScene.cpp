#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "Transform.h"


void GameScene::Initialize() 
{
    BaseGameScene::Initialize();
}

void GameScene::Finalize()
{
    BaseGameScene::Finalize();
}

void GameScene::UpdateExtra()
{
       
}

void GameScene::DrawExtra()
{
    
}

void GameScene::LoadStage()
{}

void GameScene::OnClear()
{
    GameScene::Clear();
}

void GameScene::Clear()
{
    // コントローラー入力を取得
    XINPUT_STATE joyState{};
    bool stickRightTrigger = false;
    bool stickLeftTrigger = false;

    if (Input::GetInstance()->GetJoyStick(0, joyState)) {
        float stickX = (float)joyState.Gamepad.sThumbLX / kStickMax;

        if (std::abs(stickX) > kDeadZone) {

            // 右に倒した瞬間
            if (stickX > 0.5f) {
                if (!isStickPushed) {
                    stickRightTrigger = true; // 倒した瞬間だけオン
                    isStickPushed = true;
                }
            }
            // 左に倒した瞬間
            else if (stickX < -0.5f) {
                if (!isStickPushed) {
                    stickLeftTrigger = true; // 倒した瞬間だけオン
                    isStickPushed = true;
                }
            }
            // スティックが中央に戻ったらリセット
            else {
                isStickPushed = false;
            }
        }
    }

    if (t_ < 1.0f)
    {
        t_ += 0.01f; // tを徐々に増加させる
        // カメラをプレイヤーの前へ
        Vector3 cameraPos = camera->GetTranslate();
        Vector3 newPos = Vector3Lerp(cameraPos, player_->GetPosition() + cameraOffset_, t_);
        Vector3 cameraRotate = camera->GetRotate();
        Vector3 newRotate = Vector3Lerp(player_->GetForward(), Vector3{ 0.0f,3.0f,0.0f }, t_);
        camera->SetTranslate(newPos);
        player_->SetForward(newRotate);

        // メニューUIの初期化
        for (int i = 1; i < 4; i++)
        {
            pauseSprite_[i]->SetPosition(Vector2{ (20.0f + (400.0f * (i - 1))), 500.0f });
        }
        Vector2 pos = pauseSprite_[1]->GetPosition();
        pos.y += 200.0f;
        pos.x -= 400.0f;
        cursorSprite_->SetPosition(pos);
        pauseIndex_ = 1;
    } else
    {
        if (isFadeStart_)
        {
            // フェードの更新
            fade_->Update();

            if (fade_->IsFinished())
            {
                if (pauseIndex_ == 1)
                {
                    SceneManager::GetInstance()->ChangeScene("TitleScene");
                } else if (pauseIndex_ == 2)
                {
                    SceneManager::GetInstance()->ChangeScene("SelectScene");
                } else if (pauseIndex_ == 3)
                {
                    // ステージナンバーを設定
                    int num = CollisionMask::GetInstance()->GetCurrentStageID();

                    int maxNum = static_cast<int>(CollisionMask::GetInstance()->GetMaxStageID());

                    if (num == maxNum)
                    {
                        CollisionMask::GetInstance()->SetCurrentStageID(0);
                    } else
                    {
                        CollisionMask::GetInstance()->SetCurrentStageID(num + 1);
                    }

                    SceneManager::GetInstance()->ChangeScene("GameScene");
                }
            }
        } 
        else
        {
            if (Input::GetInstance()->TriggerKeyDown(DIK_RIGHTARROW) || stickRightTrigger ||
                Input::GetInstance()->TriggerKeyDown(DIK_D) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT))
            {
                if (pauseIndex_ < 3)
                {
                    pauseIndex_++;
                } else
                {
                    pauseIndex_ = 1;
                }
            } else if (Input::GetInstance()->TriggerKeyDown(DIK_LEFTARROW) || stickLeftTrigger ||
                Input::GetInstance()->TriggerKeyDown(DIK_A) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT))
            {
                if (pauseIndex_ > 1)
                {
                    pauseIndex_--;
                } else
                {
                    pauseIndex_ = 3;
                }
            } else if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
            {
                fade_->StartFadeOut(0.02f);
                isFadeStart_ = true;
            }

        }

        Vector2 pos = pauseSprite_[pauseIndex_]->GetPosition();
        pos.y -= 500.0f;
        pos.x -= 0.0f;
        cursorSprite_->SetPosition(pos);

        for (auto& pauseSprite : pauseSprite_)
        {
            pauseSprite->Update();
        }
        menuSprite_->Update();
        cursorSprite_->Update();

    }
    
}
