#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"
#include "PlayerState.h"
#include "CollisionMask.h"

class ThreadManager;

class Player {
public:
    void Initialize(const Vector3& pos, ThreadManager* thread);
    void Finalize();
    void Update();
    void Draw();

    void UpdateMove(Vector3& moveDirection);
    void ResultMove();
    void IsCollisionSDF();
    void ChangeState(std::unique_ptr<IPlayerState> newState);
    void FireThread();

public:
    Vector3 GetPosition() const { return object_->GetTranslate(); }
    void SetPosition(const Vector3& pos);

    Vector3 GetForward() const;
    bool OnThread() const { return onThread_; }

    AABB GetAABB() const;
    Matrix4x4 GetWorldMatrix() const;

private:
    bool TryMoveOnThread(const Vector3& moveDirection);
    void ResolveThreadMove();
    void TurnToDirection(const Vector3& direction);

    void ResolveSDFCollision(float collisionThreshold);
    void StartThreadExitGrace(float exitY, const Vector3& exitPos);
    bool CanUseRelaxedGroundCollision() const;

private:
    std::unique_ptr<IPlayerState> state_;
    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    Vector3 rotate_ = {0.0f, 0.0f, 0.0f};
    Vector3 translate_ = {0.0f, 0.0f, 0.0f};

    float rotationY_ = 0.0f;
    static inline const float kTurnSpeed = 0.15f;

    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;

    float speed_ = 0.2f;
    Vector3 velocity_ = {0.05f, 0.0f, 0.05f};
    Vector3 moveVel_;

    ThreadManager* thread_ = nullptr;

private:
    bool onThread_ = false;

    Vector3 threadStart_ = {0.0f, 0.0f, 0.0f};
    Vector3 threadEnd_ = {0.0f, 0.0f, 0.0f};

    // Thread判定パラメータ
    static inline const float kThreadEnterRadius = 0.55f;
    static inline const float kThreadStickRadius = 0.5f;
    static inline const float kThreadWeightRadius = 0.90f;
    static inline const float kThreadWeight = 0.06f;

    static inline const float kThreadExitThreshold = 0.02f;
    static inline const float kThreadLateralFollowStrength = 0.65f;
    static inline const float kThreadEndSnapFadeRange = 0.18f;

    static inline const float kNormalCollisionThreshold = 0.075f;
    static inline const float kThreadExitRelaxCollisionThreshold = 0.02f;

    static inline const int kThreadExitGraceFrames = 6;
    int threadExitGraceFrames_ = 0;

    static inline const int kThreadReattachCooldownFrames = 2;
    int threadReattachCooldownFrames_ = 0;

    // 追加
    Vector3 lastThreadExitPos_ = {0.0f, 0.0f, 0.0f};
    bool hasLastThreadExitPos_ = false;

    // 追加
    static inline const float kThreadExitRelaxAreaRadius = 0.75f;

    CollisionMask::RayResult rayResult_;
};