#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ImGuiManager.h"

void TitleScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

    ku_ = std::make_unique<Object3d>();
    mo_ = std::make_unique<Object3d>();
    ri_ = std::make_unique<Object3d>();
    pressSpace_ = std::make_unique<Object3d>();
    pressA_ = std::make_unique<Object3d>();
    background_ = std::make_unique<Object3d>();

    ku_->Initialize();
    mo_->Initialize();
    ri_->Initialize();
    pressSpace_->Initialize();
    pressA_->Initialize();
    background_->Initialize();

    TextureManager::GetInstance()->LoadTexture("resources/Menu/cursor.png");
    cursor_ = std::make_unique<Sprite>();
    cursor_->Initialize("resources/Menu/cursor.png");

    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/ku.obj");
    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/mo.obj");
    ModelManager::GetInstance()->LoadModel("resources", "titleLogo/ri.obj");
    ModelManager::GetInstance()->LoadModel("resources", "pressSpace.obj");
    ModelManager::GetInstance()->LoadModel("resources", "pressA.obj");
    ModelManager::GetInstance()->LoadModel("resources", "backGround/background.obj");

    ku_->AddModel("titleLogo/ku.obj", "ku");
    mo_->AddModel("titleLogo/mo.obj", "mo");
    ri_->AddModel("titleLogo/ri.obj", "ri");
    pressSpace_->AddModel("pressSpace.obj", "pressSpace");
    pressA_->AddModel("pressA.obj", "pressA");
    background_->AddModel("backGround/background.obj", "background");

    ku_->SetTranslate(Vector3{ -2.5f,4.0f,12.0f });
    mo_->SetTranslate(Vector3{ 0.0f,4.0f,12.0f });
    ri_->SetTranslate(Vector3{ 2.5f,4.0f,12.0f });
    pressSpace_->SetTranslate(Vector3{ 0.0f,4.0f,12.0f });
    pressA_->SetTranslate(Vector3{ 0.0f,4.0f,12.0f });
    background_->SetTranslate(Vector3{ 0.0f,0.0f,66.0f });
    background_->SetScale(Vector3{4.0f, 5.0f, 1.0f});
    cursor_->SetPosition(Vector2{ 400.0f,0.0f });

    pressSpace_->SetBlendMode(BlendMode::Add);
    pressA_->SetBlendMode(BlendMode::Add);
    
    // サウンド読み込み
    handle_ = Audio::GetInstance()->LoadAudio("resources/sounds/title.wav");
    enter_ = Audio::GetInstance()->LoadAudio("resources/sounds/enter.wav");
    // サウンド再生
    Audio::GetInstance()->PlayAudio(handle_, true, 1.0f);

    webManager_ = std::make_unique<SpiderWebManager>();
    webManager_->Initialize(camera.get());
    webManager_->SetWebColor({1.0f, 1.0f, 1.0f, 1.0f});

    // 初期状態はスタート位置（左奥）にしておく
    webCurrentPos_ = webStartPos_;
    webCurrentRotation_ = webStartRotation_;

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


    float currentWebProgress = 0.0f; // 蜘蛛の巣の展開度（0.0で球体、1.0で大輪の巣）

    // ① まずはタイトルロゴ等のアニメーションを進行
    if (t_ < 1.0f) {
        t_ += 0.01f; // tを徐々に増加させる
        if (t_ > 1.0f) t_ = 1.0f;

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
        Vector3 pressA = pressA_->GetTranslate();
        pressA = Vector3Lerp(pressA, pressPos_, t_);
        pressA_->SetTranslate(pressA);
        Vector2 cursor = cursor_->GetPosition();
        cursor = Vector2Lerp(cursor, cursorPos_, t_);
        cursor_->SetPosition(cursor);

        // ロゴが動いている間は、左奥で「白い球体（展開度 0.0）」のまま待機させる
        webCurrentPos_ = webStartPos_;
        webCurrentRotation_ = webStartRotation_;
        currentWebProgress = 0.0f;
    }
    // ② タイトルのアニメーションが終わったら、蜘蛛の巣が飛び出しつつ花開く
    else if (webT_ < 1.0f) {
        webT_ += 0.015f; // スピード（シュバッと感を出したい場合は 0.02f などに）
        if (webT_ > 1.0f) webT_ = 1.0f;

        // ★ スッと気持ちよく減速して止まるEase-Out (Cubic)
        float invT = 1.0f - webT_;
        float easeWebT = 1.0f - (invT * invT * invT);

        // --- 蜘蛛の巣の移動と回転 ---
        webCurrentPos_ = Vector3Lerp(webStartPos_, webTargetPos_, easeWebT);

        // ★ 回転の補間を Vector3Lerp に変更
        webCurrentRotation_ = Vector3Lerp(webStartRotation_, webTargetRotation_, easeWebT);

        float currentScale = 4.5f * easeWebT;
        currentWebProgress = easeWebT;

        webManager_->UpdateStandaloneXY(webCurrentPos_, currentScale, webCurrentRotation_, currentWebProgress);
    } else {
        // アニメーション完了後
        webCurrentPos_ = webTargetPos_;
        webCurrentRotation_ = webTargetRotation_;
        currentWebProgress = 1.0f;

        webManager_->UpdateStandaloneXY(webCurrentPos_, 4.9f, webCurrentRotation_, currentWebProgress);
    }

    // ★ 毎フレーム必ず更新を呼ぶことで、急に出現するバグを完全回避
    // サイズを 5.0f から 4.5f にして、ロゴを邪魔しない程よい大きさに調整しました
    webManager_->UpdateStandaloneXY(webCurrentPos_, 4.9f, webCurrentRotation_, currentWebProgress);

    cursor_->Update();
    ku_->Update();
    mo_->Update();
    ri_->Update();
    pressSpace_->Update();
    pressA_->Update();
    background_->Update();

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
    background_->Draw();
    webManager_->Draw();
    ku_->Draw();
    mo_->Draw();
    ri_->Draw();

    if (Input::GetInstance()->GetConnectedStickNum() == 0)
    {
        pressSpace_->Draw();
    }
    else
    {
        pressA_->Draw();
    }

    cursor_->Draw();

    fade_->Draw();
}