#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "PathFinder.h"
#include "DrawFunction.h"
#include <deque>
#include <memory>
#include "OneWayObject.h"
#include "BrokenBlock.h"
#include "Audio.h"
#include "EnemyAnima.h"
#include "EnemyWebEffect.h"

class ThreadManager;
class Egg;
class BaseGameScene;

class Enemy {
public:
    void Initialize(const Vector3& pos);

    // GameSceneから必要な情報を毎フレーム受け取る
    void Update(const Vector3& eggPos, ThreadManager* tm, 
        const std::vector<std::unique_ptr<OneWayObject>>& oneWays, 
        const std::vector<std::unique_ptr<BrokenBlock>>& brokenBlock,
        std::vector<uint64_t>& occupiedWebKeys);

    void Draw();

    // 経路探索
    void RecalculatePath(const Vector3& eggPos, ThreadManager* tm,
        const std::vector<std::unique_ptr<OneWayObject>>& oneWays, const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock);

    // ワールド座標を取得
    Vector3 GetWorldPosition() const;

    // AABBを取得
    AABB GetAABB() const;

    // 衝突応答
    void OnCollision(Egg* egg_);

    // ヒットフラグをセット
    void SetHitFlag(bool isHit) {
        isHit_ = isHit;
    }

    void SetPosition(const Vector3& pos);

    // 外部から強制的に再計算させる
    void RequestPathReplan() {
        shouldReplanNextUpdate_ = true;
    }

    bool GetCanMove()const {
        return canMove_;
    }

    // 蜘蛛の巣に捕まっているか
    bool IsTrapped() const { return isTrapped_; }
    // 捕まっている蜘蛛の巣の識別IDを取得
    uint64_t GetTrappedWebKey() const { return trappedWebKey_; }

    void Reset(const Vector3& pos);

    // ゲームシーンのポインタをセット
    void SetGameScene(BaseGameScene* scene) { gameScene_ = scene; }

    void UpdateHeight(ThreadManager* tm); // ★追加: Y座標のみを更新する関数

    void DrawImGui();

private:
    Point WorldToGrid(const Vector3& pos);
    Vector3 GridToWorld(const Point& grid);

private:
    std::unique_ptr<EnemyAnima> anima_;
    std::unique_ptr<Object3d> object_;
    std::deque<Point> path_;

    float moveSpeed_ = 0.08f;
    int recalculateTimer_ = 0;

    std::unique_ptr<EnemyWebEffect> webEffect_;
    float webEffectTimer_ = 0.0f;
    Vector3 webEffectRot_ = {0.0f, 0.0f, 0.0f};
    
    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 卵への攻撃タイマー
    int attackTimer_ = 30;

    // 当たり判定フラグ
    bool isHit_ = false;
    bool canMove_ = true;
    bool isAttack_=false;

    bool isTrapped_ = false;   // ★蜘蛛の巣に捕まっているフラグ
    uint64_t trappedWebKey_ = 0; // ★捕まっている場所のキー

    bool shouldReplanNextUpdate_ = false; // 再計算予約フラグ

    bool isOnBridge_ = false; // 橋の上にいるかどうかの状態保持

    float weight_ = 0.01f;      // 敵の重さ（たわみ具合。ゲームに合わせて調整してください）
    float weightRadius_ = 1.0f; // 重みが影響する範囲の半径

    // サウンド
    Audio::SoundHandle attack_ = 0;

    // ゲームシーンのポインタ
    BaseGameScene* gameScene_ = nullptr;

};