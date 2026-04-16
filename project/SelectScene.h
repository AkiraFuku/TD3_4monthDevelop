#pragma once
#include "Camera.h"
#include "Audio.h"
#include "Sprite.h"
#include "Scene.h"

#include "DebugCamera.h"

class SelectScene : public Scene
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
    uint32_t handle_ = 0;

    // ステージ画像
    std::vector<std::unique_ptr <Sprite>> sprite_;
    std::unique_ptr<Sprite> arrowSprite_;
    const uint32_t kStageNum_ = 3;
    uint32_t stageIndex = 0;
    uint32_t preIndex = 0;
    std::vector<Vector2> stagePos_;
    Vector2 arrowPos_;

};

