#include "TutorialScene.h"
#include "Sprite.h"
#include "Input.h"
#include "CollisionMask.h"
#include "SceneManager.h"

void TutorialScene::Initialize()
{
    for (int i = 0; i < 3; ++i)
    {
        //std::string path = "resources/explanation/" + std::to_string(i) + ".png";

        std::string path = "resources/uvChecker.png";

        // 1つずつ生成する
        auto explanationSprite = std::make_unique<Sprite>();
        explanationSprite->Initialize(path);
        explanationSprite->SetPosition(Vector2{ 640.0f, 360.0f });
        explanationSprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f }); // 中央に配置
        explanationSprites_.push_back(std::move(explanationSprite));
    }

    objectiveSprite_ = std::make_unique<Sprite>();
    objectiveSprite_->Initialize("resources/uvChecker.png");
    objectiveSprite_->SetPosition(Vector2{ 50.0f, 160.0f });
    objectiveSprite_->SetAnchorPoint(Vector2{ 0.5f, 0.5f }); // 中央に配置

    phase_ = TutorialPhase::kExplanation;

    BaseGameScene::Initialize();

}

void TutorialScene::Finalize()
{
    explanationSprites_.clear();
    objectiveSprite_.reset();
    BaseGameScene::Finalize();
}

void TutorialScene::UpdateExtra()
{
    switch (phase_)
    {
    case TutorialPhase::kExplanation:

        BaseGameScene::UpdatePauseGray();

        if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) ||
            Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
        {
            currentPage_++;
            // 全ページ見終わったらゲーム開始
            if (currentPage_ >= static_cast<int>(explanationSprites_.size()))
            {
                currentPage_ = 0;
                phase_ = TutorialPhase::kPlaying;
            }
        }

        for (const auto& explanationSprite : explanationSprites_)
        {
            explanationSprite->Update();
        }

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
}

void TutorialScene::DrawExtra()
{
    switch (phase_)
    {
    case TutorialPhase::kExplanation:

        if (IsGameFreeze())
        {
            BaseGameScene::DrawPauseGray(); // 背景などの基本的な描画を行う

            for (const auto& explanationSprite : explanationSprites_)
            {
                explanationSprite->Draw();
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
