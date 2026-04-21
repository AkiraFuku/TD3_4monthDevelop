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
    std::unique_ptr<Sprite> sprite_;

    // フェード機能
    std::unique_ptr<Fade> fade_;

    bool isFinished_ = false;
};