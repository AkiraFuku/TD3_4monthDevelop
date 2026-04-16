#pragma once
#include "Sprite.h"
#include"Object3D.h"
#include "Camera.h"
#include "Audio.h"

#include "Scene.h"
#include <memory>

#include "DebugCamera.h"

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
     uint32_t handle_=0;
};