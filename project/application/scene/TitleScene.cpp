#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "PSOManager.h"


void TitleScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("resources/title.png");
    
    // サウンド読み込み
    handle_ = Audio::GetInstance()->LoadAudio("resources/sounds/title.wav");
    enter_ = Audio::GetInstance()->LoadAudio("resources/sounds/enter.wav");
    // サウンド再生
    Audio::GetInstance()->PlayAudio(handle_, true, 1.0f);

    fade_ = std::make_unique<Fade>();
    fade_->Initialize();
    fade_->StartFadeIn(0.05f); // シーン生成時にフェードインを開始
}
void TitleScene::Finalize() {
    Audio::GetInstance()->StopAudio(handle_);
}
void TitleScene::Update() {

    sprite_->Update();

    if (!isFinished_ &&
        (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) ||
            Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))) {

        Audio::GetInstance()->PlayAudio(enter_, false, 1.0f);
        fade_->StartFadeOut(0.05f);
        isFinished_ = true;
    }

    fade_->Update();

    if (isFinished_ && fade_->IsFinished()) {
        SceneManager::GetInstance()->ChangeScene("SelectScene");
    }
}
void TitleScene::Draw() {
    sprite_->Draw();

    fade_->Draw();
}