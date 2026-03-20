#pragma once

#include "ThreadPhysics.h"
#include "ThreadRenderer.h"
#include <Vector4.h>

#include <limits>
#include <memory>
#include <vector>

class Camera;

class ThreadManager {
public:
    struct ThreadPointInfo {
        bool hit = false;
        size_t threadIndex = (std::numeric_limits<size_t>::max)();
        size_t segmentIndex = 0;
        float segmentT = 0.0f;

        Vector3 closestPos = {0.0f, 0.0f, 0.0f};
        Vector3 tangent = {0.0f, 0.0f, 0.0f};

        float distSq = (std::numeric_limits<float>::max)();
        float distToStartSq = (std::numeric_limits<float>::max)();
        float distToEndSq = (std::numeric_limits<float>::max)();
    };

public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="maxThreads">最大描画本数</param>
    /// <param name="nodesPerThread">1本あたりのノード数</param>
    void Initialize(int maxThreads, int nodesPerThread, Camera* camera);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // プレイヤーから呼ばれる「糸を発射する」処理
    void AddThread(const Vector3& startPos, const Vector3& endPos);

    // 全ての糸を消去する
    void ClearThreads();

    /// <summary>
    /// 指定した座標の近くに糸があるかどうかを判定する
    /// </summary>
    /// <param name="pos">判定したい座標</param>
    /// <param name="radius">糸に乗っているとみなす半径</param>
    /// <returns>糸の上ならtrue</returns>
    bool IsOnThread(const Vector3& pos, float radius) const;

    /// <summary>
    /// プレイヤーや敵の重みで糸をたわませる
    /// </summary>
    void ApplyPlayerWeight(const Vector3& pos, float radius, float weight);

    /// <summary>
    /// プレイヤーや敵の座標から一番近い糸の高さを取得する
    /// </summary>
    bool GetThreadHeight(const Vector3& pos, float radius, float& outY) const;

    bool IsOnThreadWithEndRelax(const Vector3& pos,
        float centerRadius,
        float endRadius,
        int relaxSegments) const;

    bool GetThreadHeightWithEndRelax(const Vector3& pos,
        float centerRadius,
        float endRadius,
        int relaxSegments,
        float& outY) const;

    bool GetClosestThreadPointWithEndRelax(const Vector3& pos,
        float centerRadius,
        float endRadius,
        int relaxSegments,
        ThreadPointInfo& outInfo) const;

    bool GetClosestPointOnThread(size_t threadIndex,
        const Vector3& pos,
        ThreadPointInfo& outInfo) const;

    // 既存の physicsList_ を外部から読み取れるようにする
    const std::vector<std::unique_ptr<ThreadPhysics>>& GetPhysicsList() const { return physicsList_; }

private:
    // 複数本の物理演算オブジェクト
    std::vector<std::unique_ptr<ThreadPhysics>> physicsList_;

    // 描画は1つ
    std::unique_ptr<ThreadRenderer> renderer_;

    // カメラ
    Camera* camera_ = nullptr;

    int maxThreads_ = 0;
    int nodesPerThread_ = 0;
};