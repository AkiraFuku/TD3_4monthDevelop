#pragma once
#include "Camera.h"
#include "Audio.h"
#include "Object3d.h"
#include "Scene.h"
#include "DebugCamera.h"
#include "Fade.h"

#include "WorldToScreen.h"

class SelectScene : public Scene
{
public:
    void Initialize()override;
    void Finalize()override;
    void Update()override;
    void Draw()override;

    void MoveCursor();
private:
    std::unique_ptr<Camera> camera;
    DebugCamera debugCamera_;
    bool isDebugCamera_ = false;

    // ステージモデル
    std::vector<std::unique_ptr <Object3d>> objects_;
    std::vector<ScreenPosition> screenPositions_;

    std::unique_ptr<Object3d> background_;
    std::unique_ptr<Sprite> arrowSprite_;
    std::unique_ptr<Sprite> titleSprite_;
    const uint32_t kStageNum_ = 8;
    int stageIndex = -1;
    uint32_t preIndex = 0;
    std::vector<Vector3> stagePos_;
    Vector2 arrowPos_;

    // サウンド
    Audio::SoundHandle handle_ = 0;
    Audio::SoundHandle enter_ = 0;
    Audio::SoundHandle select_ = 0;

    // スティック
    bool isStickPushed_ = false;

    // フェード機能
    std::unique_ptr<Fade> fade_;

    bool isFinished_ = false;

    const float kStickMax = 32767.0f;
    const float kDeadZone = 0.3f;

    float theta = 0.0f;
    float amplitude = 1.0f;
};

