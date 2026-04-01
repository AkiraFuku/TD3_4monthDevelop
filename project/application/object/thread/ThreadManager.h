#pragma once

#include "ThreadPhysics.h"
#include "ThreadRenderer.h"
#include "Vector3.h"

#include <memory>
#include <vector>

class Camera;

class ThreadManager {
public:
    // ---------------------------------------------------------
    // 構造体定義
    // ---------------------------------------------------------

    // 糸同士の交差点情報
    struct ThreadIntersection {
        Vector3 position = {};
        float radius = 0.8f; // 交差点の判定半径

        size_t threadIndexA = 0;
        size_t threadIndexB = 0;

        size_t segmentIndexA = 0;
        size_t segmentIndexB = 0;

        Vector3 segmentAStart = {};
        Vector3 segmentAEnd = {};

        Vector3 segmentBStart = {};
        Vector3 segmentBEnd = {};
    };

    // 糸との当たり判定結果
    struct ThreadQueryResult {
        Vector3 closestPoint = {}; // 最も近い座標
        Vector3 startPoint = {};   // 該当する糸の始点
        Vector3 endPoint = {};     // 該当する糸の終点

        Vector3 segmentStart = {}; // ヒットした線分の始点
        Vector3 segmentEnd = {};   // ヒットした線分の終点

        float t = 0.0f;            // 糸全体におけるヒット位置の割合(0.0 ~ 1.0)
    };

    struct ConstrainedMoveResult {
        Vector3 velocityCorrection; // 速度に足すべき補正ベクトル
        float edgeFade;             // 糸の端にいる度合い (端:0.0f ~ 中央:1.0f)
    };

public:
    // ---------------------------------------------------------
    // ライフサイクル
    // ---------------------------------------------------------

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="maxThreads">生成可能なThreadの最大数</param>
    /// <param name="nodesPerThread">Threadのノード数</param>
    /// <param name="camera">Cameraのポインタ</param>
    void Initialize(int maxThreads, int nodesPerThread, Camera* camera);
    /// <summary>
    /// 更新
    /// </summary>
    void Update();
    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // ---------------------------------------------------------
    // 糸の管理
    // ---------------------------------------------------------

    /// <summary>
    /// Threadを生成
    /// </summary>
    /// <param name="startPos">Threadの始点</param>
    /// <param name="endPos">Threadの終点</param>
    void AddThread(const Vector3& startPos, const Vector3& endPos);
    /// <summary>
    /// Threadをクリア
    /// </summary>
    void ClearThreads();

    /// <summary>
    /// オブジェクトが糸から落ちないようにするための「横方向の補正ベクトル」を計算する
    /// </summary>
    /// <param name="nextPos">補正前の移動予定座標</param>
    /// <param name="query">対象となる糸の判定結果</param>
    /// <returns>速度に加算すべき補正ベクトル</returns>
    ConstrainedMoveResult CalculateConstrainedVelocity(const Vector3& nextPos, const ThreadQueryResult& query) const;

public:
    // ---------------------------------------------------------
    // 判定・取得
    // ---------------------------------------------------------

    /// <summary>
    /// 指定座標から最も近い糸の情報を取得する
    /// </summary>
    /// <param name="pos">検索の中心となる基準座標</param>
    /// <param name="radius">検索対象とする判定半径</param>
    /// <param name="outResult">見つかった糸の情報が格納される出力先</param>
    /// <returns>範囲内に糸が見つかった場合は true、見つからなかった場合は false</returns>
    bool FindNearestThread(const Vector3& pos, float radius, ThreadQueryResult& outResult) const;

    /// <summary>
    /// 指定座標が糸の上にあるか判定
    /// </summary>
    /// <param name="pos">判定の基準となる座標</param>
    /// <param name="radius">判定の許容範囲となる半径</param>
    /// <returns>指定した範囲内に糸が存在すれば true、存在しなければ false</returns>
    bool IsOnThread(const Vector3& pos, float radius) const;

    /// <summary>
    /// 指定座標から指定範囲内にある糸の高さ（Y座標）を取得する
    /// </summary>
    /// <param name="pos">検索の基準となる座標</param>
    /// <param name="radius">検索対象とする判定半径</param>
    /// <param name="outY">見つかった糸の高さ（Y座標）が格納される出力先</param>
    /// <returns>範囲内に糸が見つかり、高さを取得できた場合は true、見つからなかった場合は false</returns>
    bool GetThreadHeight(const Vector3& pos, float radius, float& outY) const;

    /// <summary>
    /// 指定された範囲内の糸に対して、重さ（下方向への力）を適用する
    /// </summary>
    /// <param name="pos">重さを加える中心となる座標</param>
    /// <param name="radius">重さが影響を及ぼす判定半径</param>
    /// <param name="weight">適用する重さ（下方向への力の大きさ）</param>
    void ApplyWeight(const Vector3& pos, float radius, float weight);

    // ---------------------------------------------------------
    // ゲッター
    // ---------------------------------------------------------

    const std::vector<ThreadIntersection>& GetIntersections() const { return intersections_; }
    const std::vector<std::unique_ptr<ThreadPhysics>>& GetPhysicsList() const { return physicsList_; }

private:
    // ---------------------------------------------------------
    // 内部処理
    // ---------------------------------------------------------

    // 物理演算を更新し、全ノード情報を収集する
    void UpdatePhysics(std::vector<std::vector<PhysicsNode>>& outAllNodes);

    // 糸同士の交差点を計算する
    void CalculateIntersections();

private:
    // ---------------------------------------------------------
    // メンバ変数
    // ---------------------------------------------------------
    std::vector<std::unique_ptr<ThreadPhysics>> physicsList_;
    std::vector<ThreadIntersection> intersections_;
    std::unique_ptr<ThreadRenderer> renderer_;

    Camera* camera_ = nullptr;
    int maxThreads_ = 0;
    int nodesPerThread_ = 0;

    // Threadの吸着 / 補正パラメータ
    static inline const float kThreadLateralFollowStrength = 0.65f; // 横ズレ補正の強さ
    static inline const float kThreadEndSnapFadeRange = 0.18f;      // 端で補正を弱める範囲
};