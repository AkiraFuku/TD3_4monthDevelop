#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "PSOManager.h"

void TitleScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });
}
void TitleScene::Finalize() {

}
void TitleScene::Update() {

    // スペースキーを押していたら
    if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE)) {

        Audio::GetInstance()->StopAudio(handle_);

        // ゲームシーンに戻る
        SceneManager::GetInstance()->ChangeScene("SelectScene");

    }


}
void TitleScene::Draw() {

}