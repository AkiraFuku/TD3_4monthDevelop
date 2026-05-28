#pragma once
#include "Audio.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "Egg.h"
#include "Goal.h"
#include "NestMaterial.h"
#include "MathFunction.h"
#include "Model.h"
#include "Object3D.h"
#include "ParicleEmitter.h"
#include "Player.h"
#include "Scene.h"
#include "Sprite.h"
#include "Terrain.h"
#include "TextureManager.h"
#include "Enemy.h"
#include <memory>
#include "CollisionMask.h"

#include "ThreadManager.h"
#include "SpiderWebManager.h"
#include "OneWayObject.h"
#include "BrokenBlock.h"
#include "Fade.h"
#include "StageModel.h"


class BaseGameScene : public Scene {
public:

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

    void CheckAllCollisions(); // 全ての当たり判定を確認
    bool isCollision(const AABB& aabb1, const AABB& aabb2); // 当たり判定
    bool isCollisionXZ(const AABB& aabb1, const AABB& aabb2); // XZ平面のみの当たり判定
    void ResolveCollision(Player* player, const AABB& playerAABB, const AABB& otherAABB); // プレイヤーを押し戻す関数
    void ResolveCollision(Enemy* enemy, const AABB& enemyAABB, const AABB& otherAABB); // 敵を押し戻す関数

    // クリアフラグ
    bool IsClear() const {
        return isClear_;
    }
    void SetClear(bool isClear) {
        isClear_ = isClear;
    }
    // クリア時の処理
    void Clear();

    // ポーズメニュー処理
    void Pause();

    // ゲームオーバー処理
    void GameOver();

    // setter
    void SetPauseIndex(int index) {
        pauseIndex_ = index;
    }

    bool OpenPause()const { return openPause_; }

public:

    void UpdatePauseGray();
    void DrawPauseGray();
    virtual void OnClear() = 0;
    virtual bool IsGameFreeze() const {
        return false;
    }
    virtual void UpdateExtra() = 0;
    virtual void DrawExtra() = 0;
    virtual void LoadStage() = 0;
    virtual bool IfPause() = 0;

    void ShowStuck() {
        if (!isShowStuck_) {
            isShowStuck_ = true;
            stuckAnimTime_ = 0.0f;
        }
    }

public:

    void LoadStageData();

protected:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Sprite> sprite;
    std::unique_ptr<Object3d> object3d2;
    std::unique_ptr<Object3d> object3d;
    std::unique_ptr<ParicleEmitter> emitter;


    DebugCamera debugCamera_;
    bool isDebugCamera_ = false;

    Vector3 debugSaveCameraTranslate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 debugSaveCameraRotation_ = { 0.0f, 0.0f, 0.0f };

    /*Player *player_;
    Terrain *terrain_;*/

    // 卵
    std::unique_ptr<Egg> egg_;
    // 卵の位置
    Vector3 eggPos = { -13.0f,0.0f,0.0f };

    // ゴール
    std::unique_ptr<Goal> goal_;
    // ゴールの位置
    Vector3 goalPos = { 14.0f,0.0f,0.0f };

    /*Player* player_;
    Terrain* terrain_;*/

    CollisionMask* collisionMask_;
    bool isVisibleCollisionMask_ = true;

    std::unique_ptr<StageModel> stageModel_ = nullptr;

    // ----- Player -----
    std::unique_ptr<Player> player_;

    // プレイヤーの位置
    Vector3 playerPos_ = { -13.0f, -0.4f, 0.0f };

    // ----- Thread -----
    std::unique_ptr<ThreadManager> thread_;
    std::unique_ptr<SpiderWebManager> spiderWeb_;

    // 糸の発射回数上限
    int threadLimit_ = 0;

    // 敵
    //std::unique_ptr<Enemy> enemy_;
    //// 敵の位置
    //Vector3 enemyPos_ = { 3.0f,0.0f,10.0f };

    std::vector<std::unique_ptr<Enemy>> enemies_; // 敵のリスト
    std::vector<Vector3> enemyPositions_; // 複数体の初期位置リスト

    // 巣の素材
    std::vector<std::unique_ptr<NestMaterial>> nestMaterial_;
    // 素材の位置
    std::vector<Vector3> nestMaterialPositions_;

    // 一方通行のオブジェクト
    std::vector<std::unique_ptr<OneWayObject>> stageOneWays_;

    // 数回で壊れるオブジェクト
    std::vector<std::unique_ptr<BrokenBlock>> brokenBlocks_;
    // オブジェクトの位置
    std::vector<Vector3> brokenBlockPos_;

    // サウンド
    Audio::SoundHandle handle_ = 0;
    Audio::SoundHandle enter_ = 0;
    Audio::SoundHandle select_ = 0;

    // UI
    std::vector<std::unique_ptr <Sprite>> threadLimitSprites_;
    std::vector<std::unique_ptr <Sprite>> threadCountSprites_;
    std::vector<std::unique_ptr <Sprite>> nestMaterialSprites_;
    std::vector<std::unique_ptr <Sprite>> nestCountSprites_;
    std::unique_ptr<Sprite> slashSprite_;
    std::unique_ptr<Sprite> slashNestSprite_;
    std::unique_ptr<Sprite> threadIconSprite_;
    std::unique_ptr<Sprite> nestIconSprite_;
    std::unique_ptr<Sprite> eggSprite_;
    std::unique_ptr<Sprite> hpSprite_;
    std::unique_ptr<Sprite> clearSprite_;

    std::vector < std::unique_ptr<Sprite>> pauseSprite_;
    std::unique_ptr<Sprite> menuSprite_;
    std::unique_ptr<Sprite> cursorSprite_;


    // フレーム
    std::unique_ptr<Sprite> frameSprite_;
    // 失敗時に「巣の素材が足りません！」
    std::unique_ptr<Sprite> stuckSprite_;
    bool isShowStuck_ = false;
    Vector2 stuckSpriteOriginalSize_ = { 0.0f, 0.0f };
    float stuckAnimTime_ = 0.0f;
    std::unique_ptr<Sprite> rKeySprite_;
    std::unique_ptr<Sprite> resetButtonSprite_;


    std::vector <std::unique_ptr<Sprite>> buttonSprite_;
    std::unique_ptr<Sprite> keyboard_;
    std::unique_ptr<Sprite> pad_;


    // フェード機能
    std::unique_ptr<Fade> fade_;

    // フェードアウトしてからリセットするためのフラグ
    bool isResetWaiting_ = false;

    // 背景
    std::unique_ptr<Object3d> backgroundModel_;
    Vector4 backgroundColor_ = {0.230f, 0.195f, 0.099f, 1.0f};


    // リセットフラグ
    bool isReset_ = false;
    // クリアフラグ
    bool isClear_ = false;
    bool isFadeStart_ = false;
    // ポーズメニューフラグ
    bool openPause_ = false;
    int pauseIndex_ = 0;

    // スティック
    bool isStickPushed = false;

    // カメラのオフセット
    Vector3 cameraOffset_ = { 0.0f, 10.0f, -10.0f };
    // 線形補間用の係数
    float t_ = 0.0f;


    const float kStickMax = 32767.0f;
    const float kDeadZone = 0.3f;

    int num = 0;

    int maxNum = 0;

private:
};
