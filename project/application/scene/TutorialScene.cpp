#include "TutorialScene.h"
#include "Sprite.h"
#include "Input.h"
#include "CollisionMask.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "AlphaUtility.h"

void TutorialScene::Initialize()
{

    for (int i = 0; i < explanationNum_[kTutorialMaxStage + 1]; ++i)
    {
        std::string path = "resources/tutorial/tutorial" + std::to_string(i) + ".png";

        //path = "resources/tutorial/tutorial0.png";

        auto explanationSprite = std::make_unique<Sprite>();
        explanationSprite->Initialize(path);
        explanationSprite->SetPosition(Vector2{ 640.0f, kOffscreenY });
        explanationSprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f }); // 中央に配置
        explanationSprite->SetColor(Vector4{ 0.7f, 0.7f, 0.7f, 1.0f }); 
        explanationSpritesKeyboard_.push_back(std::move(explanationSprite));
    }

    moveKey_ = std::make_unique<Sprite>();
    moveKey_->Initialize("resources/tutorial/moveKeyboard.png");
    moveKey_->SetPosition({640.0f, kOffscreenY});
    moveKey_->SetAnchorPoint({0.5f, 0.5f});

    movePad_ = std::make_unique<Sprite>();
    movePad_->Initialize("resources/tutorial/movePad.png");
    movePad_->SetPosition({640.0f, kOffscreenY});
    movePad_->SetAnchorPoint({0.5f, 0.5f});

    liftKey_ = std::make_unique<Sprite>();
    liftKey_->Initialize("resources/tutorial/liftKeyboard.png");
    liftKey_->SetPosition({640.0f, kOffscreenY});
    liftKey_->SetAnchorPoint({0.5f, 0.5f});

    liftPad_ = std::make_unique<Sprite>();
    liftPad_->Initialize("resources/tutorial/liftPad.png");
    liftPad_->SetPosition({640.0f, kOffscreenY});
    liftPad_->SetAnchorPoint({0.5f, 0.5f});

    animTimer_ = 0.0f;
    animState_ = AnimState::kAnimIn;

    objectiveSprite_ = std::make_unique<Sprite>();
    objectiveSprite_->Initialize("resources/uvChecker.png");
    objectiveSprite_->SetPosition(Vector2{ 50.0f, 160.0f });
    objectiveSprite_->SetAnchorPoint(Vector2{ 0.5f, 0.5f }); // 中央に配置

    phase_ = TutorialPhase::kExplanation;

    int currentStage = CollisionMask::GetInstance()->GetCurrentStageID();
    currentPage_ = explanationNum_[currentStage];

    tutorialTextSprite_ = std::make_unique<Sprite>();
    tutorialTextSprite_->Initialize("resources/tutorialText.png");
    tutorialTextSprite_->SetPosition(Vector2{ 400.0f, 50.0f });
    tutorialTextSprite_->SetAnchorPoint(Vector2{ 0.5f, 0.5f });

    elapsedTime_ = 0.0f;

    tutorialTextSprite_->SetColor(Vector4{ 1.0f, 1.0f, 1.0f, elapsedTime_ });

    BaseGameScene::Initialize();

}

void TutorialScene::Finalize()
{
    explanationSpritesKeyboard_.clear();
    objectiveSprite_.reset();
    BaseGameScene::Finalize();
}

void TutorialScene::UpdateExtra()
{
    const float deltaTime = 1.0f / 60.0f;
    int currentStage = CollisionMask::GetInstance()->GetCurrentStageID();

    switch (phase_)
    {
    case TutorialPhase::kExplanation:

        BaseGameScene::UpdatePauseGray();

        switch (animState_)
        {
        case AnimState::kAnimIn:
        {
            animTimer_ += deltaTime / kAnimInDuration;
            animTimer_ = std::min(animTimer_, 1.0f);

            // EaseOutBounce で Y座標を計算
            float eased = EaseOutBounce(animTimer_);
            float posY = kOffscreenY + (kTargetY - kOffscreenY) * eased;
            explanationSpritesKeyboard_[currentPage_]->SetPosition(Vector2{ 640.0f, posY });

            float movePosY = kOffscreenY + (kMoveTargetY - kOffscreenY) * eased;
            float liftPosY = kOffscreenY + (kLiftTargetY - kOffscreenY) * eased;

            switch(currentPage_)
            {
            case 0:
                moveKey_->SetPosition(Vector2{640.0f, movePosY});
                movePad_->SetPosition(Vector2{640.0f, movePosY});
                break;
            case 1:
                liftKey_->SetPosition(Vector2{640.0f, liftPosY});
                liftPad_->SetPosition(Vector2{640.0f, liftPosY});
                break;
            }

            if (animTimer_ >= 1.0f)
            {
                // 定位置に固定して入力待ちへ
                explanationSpritesKeyboard_[currentPage_]->SetPosition(Vector2{ 640.0f, kTargetY });

                switch (currentPage_) {
                case 0:
                    moveKey_->SetPosition(Vector2{640.0f, kMoveTargetY});
                    movePad_->SetPosition(Vector2{640.0f, kMoveTargetY});
                    break;
                case 1:
                    liftKey_->SetPosition(Vector2{640.0f, kLiftTargetY});
                    liftPad_->SetPosition(Vector2{640.0f, kLiftTargetY});
                    break;
                }
                animState_ = AnimState::kWaiting;
            }
            break;
        }

        case AnimState::kWaiting:
            // ボタンを押したらアニメアウト開始
            if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) ||
                Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
            {
                animTimer_ = 0.0f;
                animState_ = AnimState::kAnimOut;
            }
            break;

        case AnimState::kAnimOut:
        {
            animTimer_ += deltaTime / kAnimOutDuration;
            animTimer_ = std::min(animTimer_, 1.0f);

            // EaseInBack で Y座標を計算（定位置 → 画面外上部へ）
            float eased = EaseInBack(animTimer_);
            float posY = kTargetY + (kOffscreenY - kTargetY) * eased;
            explanationSpritesKeyboard_[currentPage_]->SetPosition(Vector2{ 640.0f, posY });

            float movePosY = kMoveTargetY + (kOffscreenY - kMoveTargetY) * eased;
            float liftPosY = kLiftTargetY + (kOffscreenY - kLiftTargetY) * eased;
            switch(currentPage_)
            {
            case 0:
                moveKey_->SetPosition(Vector2{640.0f, movePosY});
                movePad_->SetPosition(Vector2{640.0f, movePosY});
                break;
            case 1:
                liftKey_->SetPosition(Vector2{640.0f, liftPosY});
                liftPad_->SetPosition(Vector2{640.0f, liftPosY});
                break;
            }

            if (animTimer_ >= 1.0f)
            {
                
                // 次のページへ、またはゲーム開始
                currentPage_++;
                if (currentPage_ >= explanationNum_[currentStage + 1])
                {
                    phase_ = TutorialPhase::kPlaying;
                } else
                {
                    // 次ページを画面外上部にリセットしてアニメイン開始
                    explanationSpritesKeyboard_[currentPage_]->SetPosition(Vector2{ 640.0f, kOffscreenY });

                    moveKey_->SetPosition(Vector2{640.0f, kOffscreenY});
                    movePad_->SetPosition(Vector2{640.0f, kOffscreenY});
                    liftKey_->SetPosition(Vector2{640.0f, kOffscreenY});
                    liftPad_->SetPosition(Vector2{640.0f, kOffscreenY});

                    animTimer_ = 0.0f;
                    animState_ = AnimState::kAnimIn;
                }
            }
            break;
        }
        }


        for (const auto& explanationSprite : explanationSpritesKeyboard_)
        {
            explanationSprite->Update();
        }

        if (moveKey_) moveKey_->Update();
        if (movePad_) movePad_->Update();
        if (liftKey_) liftKey_->Update();
        if (liftPad_) liftPad_->Update();

        break;

    case TutorialPhase::kPlaying:
        if (IsClear())
        {
            phase_ = TutorialPhase::kFinished;
        }
        break;

    case TutorialPhase::kFinished:
        break;
    }

#ifdef USE_IMGUI

    ImGui::Begin("TutorialScene Debug");
    
    Vector2 tutorialTextPos = tutorialTextSprite_->GetPosition();
    if(ImGui::DragFloat2("Tutorial Text Position", &tutorialTextPos.x, 0.1f))
    {
        tutorialTextSprite_->SetPosition(tutorialTextPos);
    }

    ImGui::End();

#endif

    float tutorialTextAlpha = tutorialTextSprite_->GetColor().w;
    if (phase_ != TutorialPhase::kExplanation)
    {
        elapsedTime_ += deltaTime;
        if (elapsedTime_ >= 5.0f)
        {
            elapsedTime_ = 0.0f;
        }

        tutorialTextAlpha = BlinkAlpha(elapsedTime_, 0.2f);
    }

    tutorialTextSprite_->SetColor(Vector4{ 1.0f, 1.0f, 1.0f, tutorialTextAlpha });

    tutorialTextSprite_->Update();

}

void TutorialScene::DrawExtra()
{
    switch (phase_)
    {
    case TutorialPhase::kExplanation:

        if (IsGameFreeze())
        {
            BaseGameScene::DrawPauseGray(); // 背景などの基本的な描画を行う

            for (const auto& explanationSprite : explanationSpritesKeyboard_)
            {
                explanationSprite->Draw();
            }

             if (Input::GetInstance()->GetConnectedStickNum() == 0) {
                 if (currentPage_ == 0) {
                     moveKey_->Draw();
                 } else if (currentPage_ == 1) {
                     liftKey_->Draw();
                 }
             } else {
                 if (currentPage_ == 0) {
                     movePad_->Draw();
                 } else if (currentPage_ == 1) {
                     liftPad_->Draw();
                 }
             }
        }
        
        break;

    case TutorialPhase::kPlaying:
       
        // 目的テキスト
        if (objectiveSprite_)
        {
            objectiveSprite_->Draw();
        }
        break;

    case TutorialPhase::kFinished:
        // 何も描画しない
        break;
    }

    tutorialTextSprite_->Draw();
}

void TutorialScene::LoadStage()
{
    
}

void TutorialScene::OnClear()
{
    int current = CollisionMask::GetInstance()->GetCurrentStageID();

    if (current < kTutorialStageID + kTutorialMaxStage)
    {
        if (isFadeStart_)
        {
            // フェードの更新
            fade_->Update();

            if (fade_->IsFinished())
            {
                // 次のチュートリアルステージへ即遷移（演出なし）
                CollisionMask::GetInstance()->SetCurrentStageID(current + 1);
                SceneManager::GetInstance()->ChangeScene("TutorialScene");
            }
        }
        else
        {
            fade_->StartFadeOut(0.02f);
            isFadeStart_ = true;
        }
    } 
    else
    {
        BaseGameScene::Clear();
        // チュートリアル全ステージクリア → タイトルかセレクト画面へ
        //SceneManager::GetInstance()->ChangeScene("SelectScene");
    }
}

bool TutorialScene::IsGameFreeze() const
{
    if (BaseGameScene::fade_->IsFinished())
    {
        return phase_ == TutorialPhase::kExplanation;
    }

    return false;
    
}

float TutorialScene::EaseOutBounce(float t) const
{
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1)
        return n1 * t * t;
    else if (t < 2.0f / d1)
    {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1)
    {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else
    {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

// 引っ張り戻される：後ろに少し引いてから加速
float TutorialScene::EaseInBack(float t) const
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

bool TutorialScene::IfPause()
{
    if (phase_ == TutorialPhase::kPlaying)
    {
        return true;
    }

    return false;
}
