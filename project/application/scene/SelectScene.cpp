#include "SelectScene.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "CollisionMask.h"
#include "Object3dCommon.h"
#include "WinApp.h"
#include "ImGuiManager.h"
#include "Vector4Function.h"

void SelectScene::Initialize()
{
    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

    // テクスチャの読み込み
    stagePos_ =
    {
        {-3.5f,1.0f,12.0f},
        {-1.0f,1.0f,12.0f},
        {1.5f,1.0f,12.0f},
        {4.0f,1.0f,12.0f},
        { -3.5f,-1.0f,12.0f },
        { -1.0f,-1.0f,12.0f },
        {1.5f,-1.0f,12.0f},
        {4.0f,-1.0f,12.0f}
    };

    camera->Update();
    camera->UpdateViewProjection();

    for (uint32_t i = 0; i < kStageNum_; i++)
    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        std::string path = "eggSelect/" + std::to_string(i + 1) + "/egg.obj";
        object->Initialize();
        ModelManager::GetInstance()->LoadModel("resources", path);
        object->AddModel(path, "egg");
        object->SetTranslate(stagePos_[i]);
        object->SetScale(Vector3{ 0.5f,0.5f,0.5f });
        objects_.push_back(std::move(object));

        ScreenPosition screenPos;
        // ワールド座標をスクリーン座標に変換
        screenPos = WorldToScreen(objects_[i]->GetTranslate(), camera->GetViewMatrix(), camera->GetProjectionMatrix(),
            WinApp::GetInstance()->kClientWidth, WinApp::GetInstance()->kClientHeight);
        screenPositions_.push_back(screenPos);
    }

    TextureManager::GetInstance()->LoadTexture("resources/backTitle.png");
    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize("resources/backTitle.png");
    titleSprite_->SetPosition(Vector2{ 580.0f,10.0f });

    arrowSprite_ = std::make_unique<Sprite>();
    arrowSprite_->Initialize("resources/Menu/cursor.png");
    arrowSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    stageIndex = 0;
    /* Vector2 pos;
     pos.x = objects_[stageIndex]->GetTranslate().x * 100.0f;
     pos.x += 450.0f;
     pos.y = objects_[stageIndex]->GetTranslate().y - 400.0f;
     arrowSprite_->SetPosition(pos);*/

    Vector2 pos;
    pos.x = screenPositions_[stageIndex].position.x - 90.0f;
    pos.y = screenPositions_[stageIndex].position.y + 80.0f;
    arrowSprite_->SetPosition(pos);


    background_ = std::make_unique<Object3d>();
    background_->Initialize();
    ModelManager::GetInstance()->LoadModel("resources", "background/select.obj");
    background_->AddModel("background/select.obj", "background");
    background_->SetTranslate(Vector3{ 0.0f,0.0f,13.0f });
    background_->SetModelInstanceColor("background", Vector4{ 0.7f,0.7f,0.7f,1.0f });

    keyboardMenuOperation_= std::make_unique<Sprite>();
    keyboardMenuOperation_->Initialize("resources/Keyboard/keyboardMenuOperation.png");
    keyboardMenuOperation_->SetPosition(Vector2{30.0f, 620.0f});

    padMenuOperation_=std::make_unique<Sprite>();
    padMenuOperation_->Initialize("resources/Pad/padMenuOperation.png");
    padMenuOperation_->SetPosition(Vector2{30.0f, 620.0f});

    // キーボード用キースプライトの初期化
    keyboardW_ = std::make_unique<Sprite>();
    keyboardW_->Initialize("resources/Keyboard/Keyboard_WASD_0.png");
    keyboardW_->SetPosition(Vector2{ 52.7f, 621.0f });

    keyboardA_ = std::make_unique<Sprite>();
    keyboardA_->Initialize("resources/Keyboard/Keyboard_WASD_1.png");
    keyboardA_->SetPosition(Vector2{ 20.0f, 653.7f });

    keyboardS_ = std::make_unique<Sprite>();
    keyboardS_->Initialize("resources/Keyboard/Keyboard_WASD_2.png");
    keyboardS_->SetPosition(Vector2{ 52.6f, 686.7f });

    keyboardD_ = std::make_unique<Sprite>();
    keyboardD_->Initialize("resources/Keyboard/Keyboard_WASD_3.png");
    keyboardD_->SetPosition(Vector2{ 86.0f, 652.7f });

    keyboardSpace_ = std::make_unique<Sprite>();
    keyboardSpace_->Initialize("resources/Keyboard/Keyboard_3.png");
    keyboardSpace_->SetPosition(Vector2{ 250.9f, 646.0f });

    // パッド用Aボタンスプライトの初期化
    padA_ = std::make_unique<Sprite>();
    padA_->Initialize("resources/Pad/Pad_3.png");
    padA_->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
    padA_->SetPosition(Vector2{ 290.0f, 668.0f });

    // パッド用D-pad矢印スプライトの一括初期化
    struct PadArrowConfig {
        Vector2 offset;
        float rotation;
    };
    PadArrowConfig configs[4] = {
        { { 0.0f, -32.0f }, 0.0f },               // Up
        { { 32.0f, 0.0f }, float(M_PI) / 2.0f },   // Right
        { { 0.0f, 32.0f }, -float(M_PI) },        // Down
        { { -32.0f, 0.0f }, -float(M_PI) / 2.0f }  // Left
    };

    Vector2 centerPos = { 77.1f, 670.0f };
    for (int i = 0; i < 4; i++) {
        padArrowKeys_[i] = std::make_unique<Sprite>();
        padArrowKeys_[i]->Initialize("resources/Pad/padArrowKey.png");
        padArrowKeys_[i]->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
        padArrowKeys_[i]->SetPosition(centerPos + configs[i].offset);
        padArrowKeys_[i]->SetRotation(configs[i].rotation);
    }

    // サウンド読み込み
    handle_ = Audio::GetInstance()->LoadAudio("resources/sounds/stageSelect.wav");
    enter_ = Audio::GetInstance()->LoadAudio("resources/sounds/enter.wav");
    select_ = Audio::GetInstance()->LoadAudio("resources/sounds/select.wav");
    // サウンド再生
    Audio::GetInstance()->PlayAudio(handle_, true, 1.0f);

    fade_ = std::make_unique<Fade>();
    fade_->Initialize();
    fade_->StartFadeIn(0.05f); // シーン生成時にフェードインを開始
}

void SelectScene::Finalize()
{
    Audio::GetInstance()->StopAudio(handle_);
}

void SelectScene::Update()
{
    camera->Update();
    camera->UpdateViewProjection();

    MoveCursor();

    // 選択されているモデルを動かす
    if (stageIndex >= 0 && stageIndex <= 7)
    {
        Vector3 pos = objects_[stageIndex]->GetTranslate();
        pos.y = sinf(theta) * amplitude;

        if (stageIndex >= 4)
        {
            pos.y -= 1.0f;
        }
        else
        {
            pos.y += 1.0f;
        }

        theta += float(M_PI) / 60.0f; // 1秒で1周期の速度
        objects_[stageIndex]->SetTranslate(pos);
    }

#ifdef _DEBUG

    ImGui::Begin("SelectScene Debug");

    ImGui::InputFloat2("Arrow Position", &arrowPos_.x);
    for (uint32_t i = 0; i < kStageNum_; i++)
    {
        ImGui::InputFloat2("Screen Position", &screenPositions_[i].position.x);
    }

    Vector4 color = background_->GetModelInstanceColor("background");
    if (ImGui::ColorEdit4("Background Color", &color.x))
    {
        background_->SetModelInstanceColor("background", color);
    }
    ImGui::End();

#endif

    // モデルの更新処理
    for (const std::unique_ptr <Object3d>& object : objects_)
    {
        object->Update();
    }

    InputColorSet();

    titleSprite_->Update();
    arrowSprite_->Update();
    keyboardMenuOperation_->Update();
    padMenuOperation_->Update();
    background_->Update();

    keyboardW_->Update();
    keyboardA_->Update();
    keyboardS_->Update();
    keyboardD_->Update();
    keyboardSpace_->Update();

    padA_->Update();
    for (auto& arrow : padArrowKeys_) {
        arrow->Update();
    }

    fade_->Update();
}

void SelectScene::Draw()
{
    background_->Draw();
    titleSprite_->Draw();

    for (const std::unique_ptr <Object3d>& object : objects_)
    {
        object->Draw();
    }

    arrowSprite_->Draw();

    if (Input::GetInstance()->GetConnectedStickNum() == 0) {
        keyboardMenuOperation_->Draw();
        keyboardW_->Draw();
        keyboardA_->Draw();
        keyboardS_->Draw();
        keyboardD_->Draw();
        keyboardSpace_->Draw();
    } else {
        padMenuOperation_->Draw();
        padA_->Draw();
        for (auto& arrow : padArrowKeys_) {
            arrow->Draw();
        }
    }

    fade_->Draw();
}

void SelectScene::MoveCursor()
{
    preIndex = stageIndex;

    // コントローラー入力を取得
    XINPUT_STATE joyState{};
    bool stickRightTrigger = false;
    bool stickLeftTrigger = false;
    bool stickUpTrigger = false;
    bool stickDownTrigger = false;

    if (Input::GetInstance()->GetJoyStick(0, joyState)) {
        float stickX = (float)joyState.Gamepad.sThumbLX / kStickMax;
        float stickY = (float)joyState.Gamepad.sThumbLY / kStickMax;

        // 完全に中央に戻ったらロック解除
        if (std::abs(stickX) <= kDeadZone && std::abs(stickY) <= kDeadZone)
        {
            isStickPushed_ = false;
        }
        // まだ押されていない（手を離した後の最初の入力のみ受け付ける）
        else if (!isStickPushed_)
        {
            isStickPushed_ = true;

            // 大きい方の軸のみ採用
            if (std::abs(stickX) >= std::abs(stickY))
            {
                if (stickX > kDeadZone)       stickRightTrigger = true;
                else if (stickX < -kDeadZone) stickLeftTrigger = true;
            } else
            {
                if (stickY > kDeadZone)       stickUpTrigger = true;
                else if (stickY < -kDeadZone) stickDownTrigger = true;
            }
        }
    }



    //if (Input::GetInstance()->GetJoyStick(0, joyState)) {
    //    float stickX = (float)joyState.Gamepad.sThumbLY / kStickMax;

    //    if (std::abs(stickX) > kDeadZone) {

    //        // 右に倒した瞬間
    //        if (stickX > 0.5f) {
    //            if (!isStickPushedX_) {
    //                stickUpTrigger = true; // 倒した瞬間だけオン
    //                isStickPushedX_ = true;
    //            }
    //        }
    //        // 左に倒した瞬間
    //        else if (stickX < -0.5f) {
    //            if (!isStickPushedX_) {
    //                stickDownTrigger = true; // 倒した瞬間だけオン
    //                isStickPushedX_ = true;
    //            }
    //        }
    //        // スティックが中央に戻ったらリセット
    //        else {
    //            isStickPushedX_ = false;
    //        }
    //    }
    //}



    if (Input::GetInstance()->TriggerKeyDown(DIK_RIGHTARROW) || stickRightTrigger ||
        Input::GetInstance()->TriggerKeyDown(DIK_D) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT))
    {
        stageIndex++;
        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

        if (stageIndex == 8)
        {
            stageIndex = -1;
        }
    }
    else if (Input::GetInstance()->TriggerKeyDown(DIK_LEFTARROW) || stickLeftTrigger ||
        Input::GetInstance()->TriggerKeyDown(DIK_A) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT))
    {
        stageIndex--;

        if (stageIndex == -2)
        {
            stageIndex = 7;
        }


        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

    }
    else if (Input::GetInstance()->TriggerKeyDown(DIK_UPARROW) || stickUpTrigger ||
        Input::GetInstance()->TriggerKeyDown(DIK_W) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP))
    {
        if (stageIndex <= -1)
        {
            stageIndex += 8;
        }
        else
        {
            stageIndex -= 4;

            if (stageIndex <= -1)
            {
                stageIndex = -1;
            }
        }

       

        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

    }
    else if (Input::GetInstance()->TriggerKeyDown(DIK_DOWNARROW) || stickDownTrigger ||
        Input::GetInstance()->TriggerKeyDown(DIK_S) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN))
    {
        if (stageIndex == -1)
        {
            stageIndex = 0;
        }
        else
        {
            stageIndex += 4;

            if (stageIndex >= 8)
            {
                stageIndex = -1;
            }
        }

        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

    }
    else if (!isFinished_ && Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
    {
        fade_->StartFadeOut(0.05f);
        isFinished_ = true;


    }

    if (isFinished_ && fade_->IsFinished()) {

        Audio::GetInstance()->PlayAudio(enter_, false, 1.0f);

        if (stageIndex == -1)
        {
            SceneManager::GetInstance()->ChangeScene("TitleScene");
        }
        else
        {
            // ゲームシーンに移行
            CollisionMask::GetInstance()->SetCurrentStageID(stageIndex + 4);
            SceneManager::GetInstance()->ChangeScene("GameScene");
        }

    }

    if (preIndex != stageIndex)
    {
        if (stageIndex == -1)
        {
            Vector2 pos = titleSprite_->GetPosition();
            pos.y += 50.0f;
            pos.x -= 25.0f;
            arrowSprite_->SetPosition(pos);
        }
        else
        {
            Vector2 pos;
            pos.x = screenPositions_[stageIndex].position.x - 90.0f;
            pos.y = screenPositions_[stageIndex].position.y + 80.0f;
            arrowSprite_->SetPosition(pos);
        }

        if (preIndex != -1)
        {
            Vector3 objectPos = stagePos_[preIndex];
            objects_[preIndex]->SetTranslate(objectPos);
        }
    }
}

void SelectScene::InputColorSet()
{
    auto input = Input::GetInstance();
    Vector4 pushColor = { 1.0f, 0.0f, 0.0f, 1.0f };
    Vector4 defaultColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    if (Input::GetInstance()->GetConnectedStickNum() == 0)
    {
        if (input->TriggerKeyDown(DIK_W))
        {
            keyboardW_->SetColor(pushColor);
        } else if (input->TriggerKeyUp(DIK_W))
        {
            keyboardW_->SetColor(defaultColor);
        }

        if (input->TriggerKeyDown(DIK_A))
        {
            keyboardA_->SetColor(pushColor);
        } else if (input->TriggerKeyUp(DIK_A))
        {
            keyboardA_->SetColor(defaultColor);
        }

        if (input->TriggerKeyDown(DIK_S))
        {
            keyboardS_->SetColor(pushColor);
        } else if (input->TriggerKeyUp(DIK_S))
        {
            keyboardS_->SetColor(defaultColor);
        }

        if (input->TriggerKeyDown(DIK_D))
        {
            keyboardD_->SetColor(pushColor);
        } else if (input->TriggerKeyUp(DIK_D))
        {
            keyboardD_->SetColor(defaultColor);
        }

        if (input->TriggerKeyDown(DIK_SPACE))
        {
            keyboardSpace_->SetColor(pushColor);
        } else if (input->TriggerKeyUp(DIK_SPACE))
        {
            keyboardSpace_->SetColor(defaultColor);
        }
    }
    else
    {
        if (input->TriggerPadDown(0, XINPUT_GAMEPAD_A))
        {
            padA_->SetColor(pushColor);
        } else if (input->TriggerPadUP(0, XINPUT_GAMEPAD_A))
        {
            padA_->SetColor(defaultColor);
        }

        WORD buttons[4] = {
            XINPUT_GAMEPAD_DPAD_UP,
            XINPUT_GAMEPAD_DPAD_RIGHT,
            XINPUT_GAMEPAD_DPAD_DOWN,
            XINPUT_GAMEPAD_DPAD_LEFT
        };
        for (int i = 0; i < 4; i++) {
            if (input->TriggerPadDown(0, buttons[i])) {
                padArrowKeys_[i]->SetColor(pushColor);
            } else if (input->TriggerPadUP(0, buttons[i])) {
                padArrowKeys_[i]->SetColor(defaultColor);
            }
        }
    }
}
