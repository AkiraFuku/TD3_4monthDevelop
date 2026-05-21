#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ImGuiManager.h"
#include "CollisionMask.h"

void TitleScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

    ku_ = std::make_unique<Object3d>();
    mo_ = std::make_unique<Object3d>();
    ri_ = std::make_unique<Object3d>();
    pressSpace_ = std::make_unique<Object3d>();

    ku_->Initialize();
    mo_->Initialize();
    ri_->Initialize();
    pressSpace_->Initialize();

    TextureManager::GetInstance()->LoadTexture("resources/Menu/cursor.png");
    cursor_ = std::make_unique<Sprite>();
    cursor_->Initialize("resources/Menu/cursor.png");

    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/ku.obj");
    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/mo.obj");
    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/ri.obj");
    ModelManager::GetInstance()->LoadModel("resources", "pressSpace.obj");

    ku_->AddModel("titleLogo/ku.obj", "ku");
    mo_->AddModel("titleLogo/mo.obj", "mo");
    ri_->AddModel("titleLogo/ri.obj", "ri");
    pressSpace_->AddModel("pressSpace.obj", "pressSpace");

    ku_->SetTranslate(Vector3{ -2.5f,4.0f,12.0f });
    mo_->SetTranslate(Vector3{ 0.0f,4.0f,12.0f });
    ri_->SetTranslate(Vector3{ 2.5f,4.0f,12.0f });
    pressSpace_->SetTranslate(Vector3{ 0.0f,4.0f,12.0f });
    cursor_->SetPosition(Vector2{ 400.0f,0.0f });

    
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

    camera->Update();
    camera->UpdateViewProjection();


    if (t_ < 1.0f)
    {
        t_ += 0.01f; // tを徐々に増加させる
        Vector3 pos = ku_->GetTranslate();
        pos = Vector3Lerp(pos, kuPos_, t_);
        ku_->SetTranslate(pos);
        Vector3 posmo = mo_->GetTranslate();
        posmo = Vector3Lerp(posmo, moPos_, t_);
        mo_->SetTranslate(posmo);
        Vector3 posri = ri_->GetTranslate();
        posri = Vector3Lerp(posri, riPos_, t_);
        ri_->SetTranslate(posri);
        Vector3 press = pressSpace_->GetTranslate();
        press = Vector3Lerp(press, pressPos_, t_);
        pressSpace_->SetTranslate(press);
    }

    cursor_->Update();
    ku_->Update();
    mo_->Update();
    ri_->Update();
    pressSpace_->Update();

    if (!isFinished_ &&
        (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) ||
            Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))) {

        Audio::GetInstance()->PlayAudio(enter_, false, 1.0f);
        fade_->StartFadeOut(0.05f);
        isFinished_ = true;
    }

    fade_->Update();

    if (isFinished_ && fade_->IsFinished()) {
        CollisionMask::GetInstance()->SetCurrentStageID(0);
        SceneManager::GetInstance()->ChangeScene("TutorialScene");
    }
}
void TitleScene::Draw() {
    ku_->Draw();
    mo_->Draw();
    ri_->Draw();
    pressSpace_->Draw();
    cursor_->Draw();

    fade_->Draw();
}