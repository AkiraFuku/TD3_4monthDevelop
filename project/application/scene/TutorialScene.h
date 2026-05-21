#pragma once
#include "BaseGameScene.h"

class TutorialScene : public BaseGameScene
{
public:
    void Initialize();

    void Finalize();

    void UpdateExtra() override;

    void DrawExtra() override;

    void LoadStage() override;

    void OnClear() override;

    bool IsGameFreeze() const override;

private:
    enum class TutorialPhase
    {
        kExplanation,  // フェーズ1: 説明画像表示中
        kPlaying,      // フェーズ2: ゲームプレイ中
        kFinished,     // クリア後（UI非表示）
    };
    TutorialPhase phase_ = TutorialPhase::kExplanation;

private:

    std::vector<std::unique_ptr<Sprite>> explanationSprites_;
    int currentPage_ = 0;

    std::unique_ptr<Sprite> objectiveSprite_;

    static constexpr int kTutorialStageID = 0;
    static constexpr int kTutorialMaxStage = 3; // チュートリアルのステージ数（0〜2の3ステージなら2）

};

