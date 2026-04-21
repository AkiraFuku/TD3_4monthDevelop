#pragma once
#include "Camera.h"
#include "Audio.h"
#include "Sprite.h"
#include "Scene.h"
#include "DebugCamera.h"

static const float kStickMax = 32767.0f;
static const float kDeadZone = 0.3f;

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

    // ステージ画像
    std::vector<std::unique_ptr <Sprite>> sprites_;
    std::unique_ptr<Sprite> arrowSprite_;
    std::unique_ptr<Sprite> background_;
    const uint32_t kStageNum_ = 3;
    uint32_t stageIndex = 0;
    uint32_t preIndex = 0;
    std::vector<Vector2> stagePos_;
    Vector2 arrowPos_;

    // サウンド
    Audio::SoundHandle handle_ = 0;
    Audio::SoundHandle enter_ = 0;
    Audio::SoundHandle select_ = 0;

    // スティック
    bool isStickPushed = false;
};

