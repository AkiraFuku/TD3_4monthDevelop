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


    // イージング関数
    float EaseOutBounce(float t) const;  // 降ってくる用
    float EaseInBack(float t) const;     // 引っ張り戻される用

private:
    enum class TutorialPhase
    {
        kExplanation,  // フェーズ1: 説明画像表示中
        kPlaying,      // フェーズ2: ゲームプレイ中
        kFinished,     // クリア後（UI非表示）
    };
    TutorialPhase phase_ = TutorialPhase::kExplanation;

    enum class AnimState
    {
        kAnimIn,   // 上から降ってくる
        kWaiting,  // 入力待ち
        kAnimOut,  // 上へ引っ張り戻される
    };
    AnimState animState_ = AnimState::kAnimIn;


private:

    std::vector<std::unique_ptr<Sprite>> explanationSpritesKeyboard_;
    int currentPage_ = 0;

    std::unique_ptr<Sprite> objectiveSprite_;

    static constexpr int kTutorialStageID = 0;
    static constexpr int kTutorialMaxStage = 3; // チュートリアルのステージ数（0〜2の3ステージなら2）

    int explanationNum_[5] = { 0, 2, 3, 4, 7 };


    // アニメーション進行タイマー（0.0f〜1.0f）
    float animTimer_ = 0.0f;

    // アニメーション時間（秒）
    static constexpr float kAnimInDuration = 0.7f; // 降ってくる時間
    static constexpr float kAnimOutDuration = 0.7f; // 引っ張り戻される時間

    // スプライトの最終到達Y座標（定位置）
    static constexpr float kTargetY = 360.0f;
    // 画面外の開始Y座標（上）
    static constexpr float kOffscreenY = -400.0f;
    // バウンド時のオーバーシュート量
    static constexpr float kOvershoot = 30.0f;

};

