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

}
void TitleScene::Finalize() {
    Audio::GetInstance()->StopAudio(handle_);
}
void TitleScene::Update() {

    sprite_->Update();

    // スペースキーを押していたら
    if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE)) {
        // サウンド再生
        Audio::GetInstance()->PlayAudio(enter_, false, 1.0f);

        
        // ゲームシーンに戻る
        SceneManager::GetInstance()->ChangeScene("SelectScene");

    }


}
void TitleScene::Draw() {
    sprite_->Draw();
}