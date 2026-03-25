#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"
#include"PlayerState.h"
#include "CollisionMask.h"

#include "PlayerAnima.h"
class ThreadManager;
class Egg;


class Player {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="pos">初期位置</param>
    /// <param name="threadManager">ThreadManagerのポインタ</param>
    void Initialize(const Vector3& pos, ThreadManager* thread);

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// 移動処理
    /// </summary>
    void Move(const Vector3& moveDirection);

    /// <summary>
    /// 移動距離確定
    /// </summary>
    void ResultMove();

    /// <summary>
    ///
    /// </summary>
    void IsCollisionSDF();

    /// <summary>
    /// 状態遷移
    /// </summary>
    /// <param name="newState">次の状態</param>
    void ChangeState(std::unique_ptr<IPlayerState> newState);

    /// <summary>
    /// 糸を発射する処理
    /// </summary>
    void FireThread();


    //アニメーションを変更する処理
    void ChangeAnimation(PlayerAnima::AnimationState newState) {
        if (anima_) {
            anima_->ChangeAnimation(newState);
        }
    }

    // 一回きりアニメーション終了後のリセット処理を行い、指定した状態に遷移する
    void ResetOneShotAnimationAndChangeState(PlayerAnima::AnimationState newState) {
        if (anima_) {
            anima_->ResetOneShotAnimation();
            anima_->ChangeAnimation(newState);
        }
    }
   


public: 
    // 位置
    Vector3 GetPosition() const { return object_->GetTranslate(); }
    void SetPosition(const Vector3& pos);

    // 向いている方向
    Vector3 GetForward()const;

    // 糸の上を歩いているか？
    bool OnThread()const { return onThread_; }

    // AABBを取得
    AABB GetAABB() const;

    // アフィン行列
    Matrix4x4 GetWorldMatrix() const;

    void SetEgg(Egg* egg) { egg_ = egg; }
    bool CanFireThread() const;

    // 糸を撃ったかどうかを確認し、確認したらフラグを折る関数
    bool GetAndResetDidFireThread() {
        if (didFireThread_) {
            didFireThread_ = false;
            return true;
        }
        return false;
    }

    void SetMaxThreadCount(int count) { remainingThreadCount_ = count; }
    int  GetRemainingThreadCount() const { return remainingThreadCount_; }

    // 巣の素材の回収数のgetter/setter
    void SetNestMaterial(int num) { nestMaterialNum_ += num; }
    int GetNestMaterial() const { return nestMaterialNum_; }

private:
    // 現在の状態
    std::unique_ptr<IPlayerState> state_;

    // モデル
    std::unique_ptr<Object3d> object_;

    // 卵
    Egg* egg_ = nullptr;

    // 拡縮
    Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    // 回転
    Vector3 rotate_ = {0.0f, 0.0f, 0.0f};
    // 平行移動
    Vector3 translate_ = {0.0f, 0.0f, 0.0f};

    // 現状向いているY軸の角度(単位: ラジアン)
    float rotationY_ = 0.0f;
    // 旋回速度
    static inline const float kTurnSpeed = 0.15f;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 速さ
    float speed_ = 0.2f;
    // 速度
    Vector3 velocity_ = {0.05f, 0.0f, 0.05f};
    // 実際に動く時の速度
    Vector3 moveVel_;

    // ThreadManager
    ThreadManager* thread_ = nullptr;

private:
    bool TryMoveOnThread(const Vector3& moveDirection);
    void ResolveThreadMove();
    void TurnToDirection(const Vector3& direction);

private:
    // Thread上を歩いているか
    bool onThread_ = false;

    // 今乗っているThreadの情報（デバッグや将来拡張用）
    Vector3 threadStart_ = {0.0f, 0.0f, 0.0f};
    Vector3 threadEnd_ = {0.0f, 0.0f, 0.0f};

    // Thread判定パラメータ
    static inline const float kThreadEnterRadius = 0.55f;
    static inline const float kThreadStickRadius = 1.25f;
    static inline const float kThreadWeightRadius = 0.90f;
    static inline const float kThreadWeight = 0.06f;
    static inline const float kThreadExitThreshold = 0.05f;

    static inline const float kThreadLateralFollowStrength = 0.65f; // 横ズレ補正の強さ
    static inline const float kThreadEndSnapFadeRange = 0.18f;      // 端で補正を弱める範囲

    // Threadを生成できる回数
    int remainingThreadCount_ = 0;

    // 重力の強さ
    float gravity_ = 0.008f;
    // 現在の落下速度
    float fallSpeed_ = 0.0f;
    // 糸をたわませるPlayerの「重さ」
    float weight_ = 0.3f;
    // 重さを適用する範囲（半径）
    float weightRadius_ = 1.0f;

    float threadBaseY_ = 0.0f;
    float threadOffsetY_ = 0.0f;

    CollisionMask::RayResult rayResult_;

    private :
        // アニメーション制御
        std::unique_ptr<PlayerAnima> anima_;
        void InitializeModel();

    bool didFireThread_ = false; // 糸発射フラグ

    // 巣の素材の回収数
    int nestMaterialNum_ = 0;

};
