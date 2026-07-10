#pragma once
#include "Sprite.h"
#include <memory>

class Fade {
public:
    // フェードの状態
    enum class Status {
        None,
        FadeIn,
        FadeOut
    };

    void Initialize();
    void Update();
    void Draw();

    // フェードを開始する関数
    void StartFadeIn(float speed = 0.02f);
    void StartFadeOut(float speed = 0.02f);

    // フェードが完了したかどうか
    bool IsFinished() const;
    Status GetStatus() const { return status_; }

private:
    std::unique_ptr<Sprite> sprite_;
    Status status_ = Status::None;
    float alpha_ = 0.0f;
    float fadeSpeed_ = 0.02f;
};