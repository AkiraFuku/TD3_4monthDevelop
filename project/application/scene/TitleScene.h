#pragma once
#include"MathFunction.h"
#include "Sprite.h"
#include"Object3D.h"
#include "Model.h"
#include "Camera.h"
#include "ParicleEmitter.h"
#include "Audio.h"
#include "TextureManager.h"

#include "Scene.h"
#include <memory>

#include "DebugCamera.h"

#include "Animation.h"
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
    std::unique_ptr<Sprite> sprite;
    std::unique_ptr<Object3d> object3d_;
    //std::unique_ptr<Animation> animation;
  /*  std::unique_ptr<Object3d::ModelInstance> a;
    std::unique_ptr<Object3d::ModelInstance> b;*/
    std::unique_ptr<Anima> anima;

    DebugCamera debugCamera_;
    bool isDebugCamera_ = false;
     float velocity=1.0f / 60.0f;
     uint32_t handle_=0;
};

