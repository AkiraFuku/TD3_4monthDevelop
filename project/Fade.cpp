#include "Fade.h"

void Fade::Initialize() {
    sprite_ = std::make_unique<Sprite>();

    // 単色のテクスチャを指定（プロジェクト内にある真っ黒や真っ白の画像を読み込みます）
    sprite_->Initialize("resources/white.png");

    // 画面全体を覆うサイズに設定 (解像度に合わせて変更してください。例: 1280x720)
    sprite_->SetPosition({0.0f, 0.0f}); // アンカーポイントが中心(0.5, 0.5)の場合
    sprite_->SetSize({1280.0f, 720.0f});
}

void Fade::Update() {
    if (status_ == Status::FadeIn) {
        alpha_ -= fadeSpeed_;
        if (alpha_ <= 0.0f) {
            alpha_ = 0.0f;
            status_ = Status::None;
        }
    } else if (status_ == Status::FadeOut) {
        alpha_ += fadeSpeed_;
        if (alpha_ >= 1.0f) {
            alpha_ = 1.0f;
            status_ = Status::None;
        }
    }

    // スプライトのアルファ値を更新 (RGBAのAを変更)
    // ※ SpriteクラスにSetColorのような関数があることを想定しています。
    sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha_});

    sprite_->Update();
}

void Fade::Draw() {
    // 完全に透明(alpha_ == 0.0f)のときは無駄な描画を省く
    if (alpha_ > 0.0f) {
        sprite_->Draw();
    }
}

void Fade::StartFadeIn(float speed) {
    status_ = Status::FadeIn;
    alpha_ = 1.0f; // フェードイン開始時は真っ暗(アルファ値1)
    fadeSpeed_ = speed;
}

void Fade::StartFadeOut(float speed) {
    status_ = Status::FadeOut;
    alpha_ = 0.0f; // フェードアウト開始時は透明(アルファ値0)
    fadeSpeed_ = speed;
}

bool Fade::IsFinished() const {
    return status_ == Status::None;
}