#pragma once
#include "Sprite.h"
#include"Object3D.h"
#include "Camera.h"
#include "Audio.h"

#include "Scene.h"
#include <memory>

#include "DebugCamera.h"
#include "Fade.h"

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

    // フェード機能
    std::unique_ptr<Fade> fade_;

    bool isFinished_ = false;

    // 線形補間用の係数
    float t_ = 0.0f;

    // 座標
    Vector3 kuPos_ = { -2.5f,1.5f,12.0f };
    Vector3 moPos_ = { 0.0f,1.0f,12.0f };
    Vector3 riPos_ = { 2.5f,0.5f,12.0f };
    Vector3 pressPos_ = { 0.0f,-2.0f,12.0f };
    Vector2 cursorPos_ = { 400.0f,0.0f };
};