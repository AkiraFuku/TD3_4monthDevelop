#pragma once
#include "Sprite.h"
#include"Object3D.h"
#include "Camera.h"
#include "Audio.h"

#include "Scene.h"
#include <memory>

#include "DebugCamera.h"
#include "Fade.h"
#include "SpiderWebManager.h"

#include <Anima.h>
class TitleScene :public Scene
{
public:
    void Initialize()override;
    void Finalize()override;
    void Update()override;
    void Draw()override;
private:
    std::unique_ptr<Camera> camera;
    DebugCamera debugCamera_;
    bool isDebugCamera_ = false;
    Audio::SoundHandle handle_ = 0;
    Audio::SoundHandle enter_ = 0;
    std::unique_ptr<Sprite> cursor_;
    std::unique_ptr<Object3d> ku_;
    std::unique_ptr<Object3d> mo_;
    std::unique_ptr<Object3d> ri_;
    std::unique_ptr<Object3d> pressSpace_;
    std::unique_ptr<Object3d> pressA_;
    std::unique_ptr<Object3d> background_;

    // フェード機能
    std::unique_ptr<Fade> fade_;

    bool isFinished_ = false;

    // 線形補間用の係数
    float t_ = 0.0f;
    float webT_ = 0.0f; // 蜘蛛の巣専用の係数

    // 座標
    Vector3 kuPos_ = { -2.5f,1.5f,12.0f };
    Vector3 moPos_ = { 0.0f,1.0f,12.0f };
    Vector3 riPos_ = { 2.5f,0.5f,12.0f };
    Vector3 pressPos_ = { 0.0f,-2.0f,12.0f };
    Vector2 cursorPos_ = { 250.0f,0.0f };

private:
    std::unique_ptr<SpiderWebManager> webManager_;

    // 軌道のコントロール
    Vector3 webStartPos_ = {-12.0f, 0.0f, 65.0f}; // 【始点】左・上・かなり奥
    Vector3 webTargetPos_ = {1.8f, -0.8f, 14.5f}; // 【終点】中央より右寄り・やや下・手前
    Vector3 webCurrentPos_;

    // 回転のコントロール（3D回転に変更）
    Vector3 webStartRotation_ = {0.0f, 0.0f, -4.5f}; // 開始時は主にZ軸でぐるぐる回る

    // 【目標の角度】
    // X: プラスで上向き、マイナスで下向きに傾く
    // Y: プラスで右向き、マイナスで左向きに傾く
    Vector3 webTargetRotation_ = {0.4f, -0.6f, 0.0f}; // ★ここで斜めの角度を微調整！

    Vector3 webCurrentRotation_ = {0.0f, 0.0f, 0.0f};

};