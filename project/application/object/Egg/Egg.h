#pragma once

#include"Object3D.h"
#include "Model.h"
#include "Camera.h"
#include "DrawFunction.h"
#include "Audio.h"
#include "EggAnima.h"
#include "EggExplosion.h"
#include <vector>

class Player;
class GameScene;

class Egg
{
public:

    void Initialize(const Vector3& pos);

    void Finalize();

    void Update();

    void Draw();

    // ワールド座標を取得
    Vector3 GetWorldPosition() const;

    bool IsHit() const { return isHit_; }

    // AABBを取得
    AABB GetAABB() const;

    // 衝突応答
    void OnCollision(const Player* player_);

    // ヒットフラグをセット
    void SetHitFlag(bool isHit) { isHit_ = isHit; }

    // プレイヤーのポインタ
    void SetPlayer(Player* player) { player_ = player; }
    // ゲームシーンのポインタ
    void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

    // 卵の状況を取得
    bool IsOnPlayer() const { return onPlayer_; }

    // HPを減らす
    void SetHP(float hp);
    float GetHP() const { return HP_; }

    // 死亡判定
    void Death();

    // getter
    bool IsDead() const { return isDead_; }
    
public: // 外部入出力

    void SetTranslate(const Vector3& translate) { object_->SetTranslate(translate); }

private:

    std::unique_ptr<EggAnima> anima_;
    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = { 1.0f,1.0f,1.0f };
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };
    Vector3 translate;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 6.4f;

    // 当たり判定フラグ
    bool isHit_ = false;
    // 卵を持ち上げるフラグ
    bool onPlayer_ = false;
    // プレイヤーのポインタ
    Player* player_ = nullptr;
    // ゲームシーンのポインタ
    GameScene* gameScene_ = nullptr;

    // HP
    float HP_ = 10.0f;
    bool isDead_ = false;

    // サウンド
    Audio::SoundHandle up_ = 0;
    Audio::SoundHandle down_ = 0;

    float maxHP_ = 10.0f;           // 最大HP
    float damageEffectTimer_ = 0.0f; // ダメージ点滅タイマー
    const float kDamageEffectTime = 0.5f; // 点滅させる合計時間
    float flickerCounter_ = 0.0f;    // 明滅用カウンター
    bool isDamage=false;

    // 卵の爆発エフェクト
    std::vector<std::unique_ptr<EggExplosion>> explosionEffect_;
    // 透明度
    float a = 1.0f;
};

