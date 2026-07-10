#pragma once

#include "ThreadManager.h"
#include "ThreadRenderer.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

class Camera;

class SpiderWebManager {
public:
    void Initialize(Camera* camera);
    void Update(const ThreadManager& threadManager);
    void Draw() const;

    void UpdateStandaloneXY(const Vector3& center, float scale, const Vector3& rotation, float progress);

    // 蜘蛛の巣の色を変更する
    void SetWebColor(const Vector4& color);

private:
    // --- ランダム形状キャッシュ ---
    // 蜘蛛の巣1つ分の形状ゆらぎと、毎フレームの一時バッファを保持する。
    // ワークバッファをここに持たせることでフレームごとの動的アロケーションを排除する。
    struct WebShapeCache {
        bool initialized = false;
        std::vector<float> spokeLengthRates;
        std::vector<float> ringRadiusRates;

        float animationProgress = 0.0f;

        // --- 再利用ワークバッファ（毎フレームのアロケーションを避ける） ---
        std::vector<float>   workSpokeLengths; // 放射糸長 [spokeCount]
        std::vector<Vector3> workRingPoints;   // 輪糸座標 [spokeCount]
    };

private:
    void BuildWebFromIntersection(const ThreadManager::ThreadIntersection& intersection);
    void BuildWebStandaloneXY(const Vector3& center, float scale, const Vector3& rotation, float progress, uint64_t key);
    void AddSegmentAsThread(const Vector3& start, const Vector3& end);

    float    Hash01(float x) const;
    uint64_t MakeIntersectionKey(const ThreadManager::ThreadIntersection& intersection) const;

    WebShapeCache& GetOrCreateWebShape(uint64_t key, size_t spokeCount, size_t ringCount);

    // 輪糸1スパン分の弧セグメントを追加する共通ヘルパー
    void AddRingSpanArc(
        const Vector3& start, const Vector3& end,
        const Vector3& center, float maxSag);

private:
    Camera* camera_ = nullptr;
    std::unique_ptr<ThreadRenderer> renderer_;

    std::vector<std::vector<PhysicsNode>> webNodes_;
    std::unordered_map<uint64_t, WebShapeCache> webShapeCache_;

    float spokeLength_ = 1.5f;
    float innerHoleRadius_ = 0.03f;
    float liftY_ = 0.02f;

    std::array<float, 5> ringRates_ = {0.18f, 0.34f, 0.52f, 0.73f, 0.96f};

    float webThickness_ = 0.025f;
    int   radialSegments_ = 4;
    int   maxIntersections_ = 64;

    int   arcSegments_ = 4;    // 輪糸1スパンあたりの分割数
    float arcSagFactor_ = 0.15f;

    Vector4 webColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

    float missingThreadProbability_ = 0.15f;
};