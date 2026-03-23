#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"
#include "PlayerState.h"
#include "CollisionMask.h"

class ThreadManager;
class Egg;

class Player {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="pos">初期位置</param>
    /// <param name="thread">ThreadManagerのポインタ</param>
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
    /// <param name="moveDirection">移動したい方向</param>
    void Move(const Vector3& moveDirection);
    /// <summary>
    /// 移動距離確定処理
    /// </summary>
    void ResultMove();

    /// <summary>
    /// CollisionMaskとの当たり判定
    /// </summary>
    void IsCollisionSDF();

    /// <summary>
    /// PlayerStateの切り替え処理
    /// </summary>
    /// <param name="newState"></param>
    void ChangeState(std::unique_ptr<IPlayerState> newState);

    /// <summary>
    /// Threadの生成処理
    /// </summary>
    void FireThread();

public:
    // 位置
    Vector3 GetPosition() const { return object_->GetTranslate(); }
    void SetPosition(const Vector3& pos);

    // 前方方向
    Vector3 GetForward() const;

    // Threadに乗っているのか？
    bool OnThread() const { return onThread_; }

    // Threadを生成することができる状態か？
    bool CanFireThread() const;

    // AABB
    AABB GetAABB() const;

    // ワールド行列
    Matrix4x4 GetWorldMatrix() const;

    // Egg
    void SetEgg(Egg* egg) { egg_ = egg; }

private:
    // Playerの状態
    std::unique_ptr<IPlayerState> state_;

    // モデル
    std::unique_ptr<Object3d> object_;

    // Egg
    Egg* egg_ = nullptr;

    // ===================================
    // Playerの挙動
    // ===================================

    // 重さ
    float mass_ = 1.0f;
    // 縦速度
    float verticalVelocity_ = 0.0f;
    // 通常地面の基準Y
    float groundY_ = 0.0f;
    // 糸に乗った瞬間のY差分
    float threadRideOffsetY_ = 0.0f;

    // 毎フレームかかる重力の強さ
    static inline const float kGravityAcceleration = -0.008f;
    // 落下速度の最大値
    static inline const float kMaxFallSpeed = -0.20f;

    // 拡縮
    Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    // 回転
    Vector3 rotate_ = {0.0f, 0.0f, 0.0f};
    // 平行移動
    Vector3 translate_ = {0.0f, 0.0f, 0.0f};

    // Playerが向いている角度
    float rotationY_ = 0.0f;
    // 旋回の速さ
    static inline const float kTurnSpeed = 0.15f;

    // プレイヤーの当たり判定用
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    // 移動の速さ
    float speed_ = 0.2f;
    // 速度
    Vector3 velocity_ = {0.05f, 0.0f, 0.05f};
    // 実際に動く時の速度
    Vector3 moveVel_ = {0.0f, 0.0f, 0.0f};

    // Thread
    ThreadManager* thread_ = nullptr;

private:
    /// <summary>
    /// 糸の上に移動できるか？
    /// </summary>
    /// <param name="moveDirection">移動したい方向</param>
    /// <returns></returns>
    bool TryMoveOnThread(const Vector3& moveDirection);
    /// <summary>
    /// 糸の上の移動結果を実際の座標に反映する処理
    /// </summary>
    void ResolveThreadMove();
    /// <summary>
    /// 指定した方向にPlayerを振り向かせる処理
    /// </summary>
    /// <param name="direction"></param>
    void TurnToDirection(const Vector3& direction);

private:

    // ===================================
    // Thread
    // ===================================
   
    // Threadに乗っているのか？
    bool onThread_ = false;

    // Playerが乗っているThreadの始点
    Vector3 threadStart_ = {0.0f, 0.0f, 0.0f};
    // Playerが乗っているThreadの終点
    Vector3 threadEnd_ = {0.0f, 0.0f, 0.0f};

    // 糸に飛び乗れる高さの許容範囲
    static inline const float kThreadEnterHeightTolerance = 0.60f;
    // 糸に近づいたと判定され、乗り移る基準の距離
    static inline const float kThreadEnterRadius = 0.55f;
    // 糸から落ちないように足元を吸着させる有効範囲
    static inline const float kThreadStickRadius = 1.25f;
    // プレイヤーの重みで糸が周囲に影響（たわむ）する範囲
    static inline const float kThreadWeightRadius = 0.90f;
    // mass_ (重さ) 1に対して、糸がどれくらいたわむかの係数
    static inline const float kThreadWeightPerMass = 0.06f;
    // 糸から降りた（落ちた）と判定する閾値
    static inline const float kThreadExitThreshold = 0.05f;

    // 糸が横に揺れた際、プレイヤーがどれくらいそれに引っ張られるか
    static inline const float kThreadLateralFollowStrength = 0.65f;
    // 糸の端っこに近づいた際、不自然に吸着しないよう判定をフェードアウトさせる距離
    static inline const float kThreadEndSnapFadeRange = 0.18f;

    CollisionMask::RayResult rayResult_;
};