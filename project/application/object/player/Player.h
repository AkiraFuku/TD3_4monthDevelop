#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"
#include"PlayerState.h"
#include "CollisionMask.h"
#include "Audio.h"
#include "PlayerAnima.h"
#include "Vector3.h"

#include "JSONManager.h"

#include <vector>
#include <memory>

class ThreadManager;
class Egg;
class OneWayObject;
class BrokenBlock;
class BaseGameScene;

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

    void CreatePSO();

public:
    // 位置
    Vector3 GetPosition() const { return object_->GetTranslate(); }
    void SetPosition(const Vector3& pos);

    // 向いている方向
    Vector3 GetForward()const;
    void SetForward(const Vector3& forward);

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

    // 巣の素材の回収数の getter/setter
    void SetNestMaterial(int num) { nestMaterialNum_ += num; }
    void ResetNestMaterial() { nestMaterialNum_ = 0; }
    int GetNestMaterial() const { return nestMaterialNum_; }

    /// <summary>
    /// 予測線の描画フラグを設定
    /// </summary>
    void SetCanDrawPrediction(bool canDraw) { canDrawPrediction_ = canDraw; }

    void SetOneWayObjects(const std::vector<OneWayObject*>& oneWays) {
        oneWayObjects_ = oneWays;
    }
    // OneWayObjectに乗っているかどうかのゲッターとセッター
    void SetCurrentOneWay(OneWayObject* oneWay) { currentOneWay_ = oneWay; }
    OneWayObject* GetCurrentOneWay() const { return currentOneWay_; }

    // 現在足元にOneWayObjectがあるか確認して返す関数
    OneWayObject* CheckOnOneWayObject() const;

    void SetBrokenBlocks(const std::vector<BrokenBlock*>& blocks) {
        brokenBlocks_ = blocks;
    }

    // 糸の生成回数のgetter
    int GetThreadCount() const { return remainingThreadCount_; }

    // ゲームシーンのポインタをセット
    void SetGameScene(BaseGameScene* scene) { gameScene_ = scene; }

    void UpdateHeight();

    // ---- 経路チェック機能 ----
    // ゴール座標をセット（GameSceneから渡す）
    void SetGoalPosition(const Vector3& goal) { goalPos_ = goal; }
    // 収集すべき素材の座標リストをセット
    void SetMaterialPositions(const std::vector<Vector3>& positions) { materialPositions_ = positions; }
    // OneWayObject / BrokenBlock のリストをセット（経路探索に使用）
    void SetRouteCheckObjects(
        const std::vector<std::unique_ptr<OneWayObject>>* oneWays,
        const std::vector<std::unique_ptr<BrokenBlock>>* brokenBlocks) {
        routeOneWays_ = oneWays;
        routeBrokenBlocks_ = brokenBlocks;
    }

    // 経路チェックが失敗したかどうかを取得する
    bool GetRouteCheckFailed() const { return routeCheckFailed_; }
    void SetRouteCheckFailed(bool failed) { routeCheckFailed_ = failed; }

    // ゴールに必要な素材の数をセットする
    void SetNeedNestMaterialCount(int count) { needNestMaterialCount_ = count; }

    // 独自のスタック判定用経路探索 (BFS)
    bool CheckRoute();

private:
    // 現在乗っているOneWayObjectのポインタ
    OneWayObject* currentOneWay_ = nullptr;

    std::vector<BrokenBlock*> brokenBlocks_;

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

    // --- 予測線描画用 ---
    Vector3 predictedThreadStart_ = {0.0f, 0.0f, 0.0f};
    Vector3 predictedThreadEnd_ = {0.0f, 0.0f, 0.0f};
    bool canDrawPrediction_ = false; // 現在糸を生成可能（予測線を描画可能）か

    std::unique_ptr<Object3d> predictionLineObj_;
    std::unique_ptr<Object3d> predictionPointObj_;

    // ThreadManager
    ThreadManager* thread_ = nullptr;
    // ゲームシーンのポインタ
    BaseGameScene* gameScene_ = nullptr;

public:
    // 状態クラス（PlayerState）から回転だけを呼び出せるように public へ移動
    void TurnToDirection(const Vector3& direction);

private:
    bool TryMoveOnThread(const Vector3& moveDirection);
    void ResolveThreadMove();

    void UpdatePredictionLine();

    void IsCollisionOneWay();

    // 線分(start - end)がOneWayObjectを跨いでいるか判定する
    bool IntersectsAnyOneWayObject(const Vector3& start, const Vector3& end) const;

    // 糸の生成を防ぐための、OneWayObjectに対する接近制限マージン値
    static inline const float kMinDistanceToOneWay = 0.8f;

    // Player専用の糸生成防止用の仮想AABBを取得する
    AABB GetThreadBlockAABBForPlayer(const OneWayObject* oneWay) const;

private:
    // Thread上を歩いているか
    bool onThread_ = false;

    // 今乗っているThreadの情報（デバッグや将来拡張用）
    Vector3 threadStart_ = {0.0f, 0.0f, 0.0f};
    Vector3 threadEnd_ = {0.0f, 0.0f, 0.0f};

    // Thread判定パラメータ
    static inline const float kThreadEnterRadius = 0.55f;
    static inline const float kThreadStickRadius = 1.0f;
    static inline const float kThreadWeightRadius = 0.90f;
    static inline const float kThreadWeight = 0.06f;
    static inline const float kThreadExitThreshold = 0.00f;

    // 糸を生成する際、既存の糸とどれくらい離れていれば生成可能とするかの距離
    static inline const float kMinThreadCreateDistance = 1.0f;

    // Threadを生成できる回数
    int remainingThreadCount_ = 0;

    // 糸をたわませるPlayerの「重さ」
    float weight_ = 0.3f;
    // 重さを適用する範囲（半径）
    float weightRadius_ = 1.0f;

    float threadBaseY_ = -0.40f;
    float threadOffsetY_ = 0.0f;

    float currentEdgeFade_ = 1.0f; // ★追加: UpdateHeightで使うためにフェード値を保存

    CollisionMask::RayResult rayResult_;

    // 一方通行オブジェクトのポインタリスト
    std::vector<OneWayObject*> oneWayObjects_;

private:
    // アニメーション制御
    std::unique_ptr<PlayerAnima> anima_;
    void InitializeModel();

    bool didFireThread_ = false; // 糸発射フラグ

    // 巣の素材の回収数
    int nestMaterialNum_ = 0;

    // サウンド
    Audio::SoundHandle threadSound_ = 0;

public:
    // jsonにセーブ
    void SaveJson();
    // jsonからロード
    void LoadJson();

private:
    JSONManager::Group playerGroup_;

    // ゴール座標（GameSceneからセット）
    Vector3 goalPos_ = {0.0f, 0.0f, 0.0f};
    // 収集すべき素材の座標リスト（GameSceneからセット）
    std::vector<Vector3> materialPositions_;
    // ゴールに必要な素材の数
    int needNestMaterialCount_ = 0;

    // 経路探索に使う外部オブジェクトリスト（所有権なし・ポインタのみ保持）
    const std::vector<std::unique_ptr<OneWayObject>>* routeOneWays_ = nullptr;
    const std::vector<std::unique_ptr<BrokenBlock>>* routeBrokenBlocks_ = nullptr;

    // 「到達不可」フラグと、到達できなかった理由の文字列（ImGui表示用）
    bool        routeCheckFailed_ = false;
    std::string routeFailReason_;
};