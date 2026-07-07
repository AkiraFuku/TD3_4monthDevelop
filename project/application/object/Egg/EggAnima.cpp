#include "EggAnima.h"
#include "MathFunction.h"
#include "RotateFunction.h"
#include "Quanternion.h"
#include <cmath>

void EggAnima::Initialize(Object3d* targetObject)
{
    targetObject_ = targetObject;
     anima_ = std::make_unique<Anima>();
    anima_->Initialize(targetObject);
    InitializeAnimations();
    InitializeAnimationSpeeds();
}
void EggAnima::InitializeAnimationSpeeds()
{
    animationSpeed_ = 1.0f;
}
void EggAnima::InitializeAnimations()
{
    animation_ = Anima::AnimeMove{
        [this](Object3d::ModelInstance& instance) {
            float t = std::sin(anima_->GetTimer() * 2.0f * animationSpeed_) * 0.5f + 0.5f;
            //卵をz軸左右に揺らす

            instance.transform.translate.z = Lerp(-0.1f, 0.1f, t);
      
        },
        true, false, false, -1.0f
    };
    anima_->SetCurrentMove(animation_);
}
void EggAnima::Update()
{
    anima_->Update();
  /*   float t = std::sin(anima_->GetTimer() * 2.0f * animationSpeed_) * 0.5f + 0.5f;
     Vector3 rotate = targetObject_->GetTranslate();
     rotate.z += Lerp(-0.1f, 0.1f, t);
     targetObject_->SetRotate(rotate);*/
}
void EggAnima::SetAnimationSpeed(float speed)
{
    animationSpeed_ = speed;
}