#include "Anima.h"
#include <map>
#include <memory>
class EggAnima
{

public:
    void Initialize(Object3d* targetObject);
    void Update();
    void Play() {
        anima_->Play();
    }
    void Stop() {
        anima_->Stop();
    }
     // アニメーション再生速度の設定
    void SetAnimationSpeed( float speed);
private:
    std::unique_ptr<Anima> anima_;
   Anima::AnimeMove animation_;
    float animationSpeed_;
    Object3d* targetObject_;

      // アニメーション定義用ヘルパー関数
    void InitializeAnimations();
    void InitializeAnimationSpeeds();

};