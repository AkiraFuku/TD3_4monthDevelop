#include "SelectScene.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "CollisionMask.h"
#include "Object3dCommon.h"

void SelectScene::Initialize()
{
    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

    // テクスチャの読み込み
    stagePos_ =
    {
        {-3.5f,0.0f,12.0f},
        {-1.0f,0.0f,12.0f},
        {1.5f,0.0f,12.0f},
        {4.0f,0.0f,12.0f}
    };

    for (uint32_t i = 0; i < kStageNum_; i++)
    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        std::string path = "eggSelect/" + std::to_string(i + 1) + "/egg.obj";
        object->Initialize();
        ModelManager::GetInstance()->LoadModel("resources", path);
        object->AddModel(path, "egg");
        object->SetTranslate(stagePos_[i]);
        objects_.push_back(std::move(object));
    }

    TextureManager::GetInstance()->LoadTexture("resources/backTitle.png");
    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize("resources/backTitle.png");
    titleSprite_->SetPosition(Vector2{ 0.0f,0.0f });

    arrowSprite_ = std::make_unique<Sprite>();
    arrowSprite_->Initialize("resources/Menu/cursor.png");
    stageIndex = 0;
    Vector2 pos;
    pos.x = objects_[stageIndex]->GetTranslate().x * 100.0f;
    pos.x += 450.0f;
    pos.y = objects_[stageIndex]->GetTranslate().y - 400.0f;
    arrowSprite_->SetPosition(pos);
    

    background_ = std::make_unique<Object3d>();
    background_->Initialize();
    ModelManager::GetInstance()->LoadModel("resources", "background/select.obj");
    background_->AddModel("background/select.obj", "background");
    background_->SetTranslate(Vector3{ 0.0f,0.0f,13.0f });


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
    if (stageIndex >= 0 && stageIndex <= 3)
    {
        Vector3 pos = objects_[stageIndex]->GetTranslate();
        pos.y = sinf(theta) * amplitude;
        theta += float(M_PI) / 60.0f; // 1秒で1周期の速度
        objects_[stageIndex]->SetTranslate(pos);
    }
    // モデルの更新処理
    for (const std::unique_ptr <Object3d>& object : objects_)
    {
        object->Update();
    }

    titleSprite_->Update();
    arrowSprite_->Update();
    background_->Update();

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

    fade_->Draw();
}

void SelectScene::MoveCursor()
{
    preIndex = stageIndex;

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



    if (Input::GetInstance()->TriggerKeyDown(DIK_RIGHTARROW) || stickRightTrigger ||
        Input::GetInstance()->TriggerKeyDown(DIK_D) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT))
    {
        stageIndex++;
        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

        if (stageIndex == 4)
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
            stageIndex = 3;
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
           // CollisionMask::GetInstance()->SetCurrentStageID(stageIndex + 4);
            CollisionMask::GetInstance()->SetCurrentStageID(8);
            SceneManager::GetInstance()->ChangeScene("GameScene");
        }
       
    }

    if (preIndex != stageIndex)
    {
        if (stageIndex == -1)
        {
            Vector2 pos = titleSprite_->GetPosition();
            pos.y -= 600.0f;
            arrowSprite_->SetPosition(pos);
        }
        else
        {
            Vector2 pos;
            pos.x = objects_[stageIndex]->GetTranslate().x * 100.0f;
            pos.x += 450.0f;
            pos.y = objects_[stageIndex]->GetTranslate().y - 400.0f;
            arrowSprite_->SetPosition(pos);
        }

        if (preIndex != -1)
        {
            Vector3 objectPos = stagePos_[preIndex];
            objects_[preIndex]->SetTranslate(objectPos);
        }
    }
}
