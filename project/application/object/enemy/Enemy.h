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

class ThreadManager;
class Egg;
class GameScene;

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
    void SetGameScene(GameScene* scene) { gameScene_ = scene; }

private:
    Point WorldToGrid(const Vector3& pos);
    Vector3 GridToWorld(const Point& grid);

private:
    std::unique_ptr<EnemyAnima> anima_;
    std::unique_ptr<Object3d> object_;
    std::deque<Point> path_;
    float moveSpeed_ = 0.08f;
    int recalculateTimer_ = 0;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 卵への攻撃タイマー
    int attackTimer_ = 60;

    // 当たり判定フラグ
    bool isHit_ = false;
    bool canMove_ = true;
    bool isAttack_=false;

    bool isTrapped_ = false;   // ★蜘蛛の巣に捕まっているフラグ
    uint64_t trappedWebKey_ = 0; // ★捕まっている場所のキー

    bool shouldReplanNextUpdate_ = false; // 再計算予約フラグ

    bool isOnBridge_ = false; // 橋の上にいるかどうかの状態保持

    // サウンド
    Audio::SoundHandle attack_ = 0;

    // ゲームシーンのポインタ
    GameScene* gameScene_ = nullptr;

};