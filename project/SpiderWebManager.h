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

private:
    struct WebShapeCache {
        bool initialized = false;
        std::vector<float> spokeLengthRates;
        std::vector<float> ringRadiusRates;
    };

private:
    void BuildWebFromIntersection(const ThreadManager::ThreadIntersection& intersection);
    void AddSegmentAsThread(const Vector3& start, const Vector3& end);

    float Hash01(float x) const;
    uint64_t MakeIntersectionKey(const ThreadManager::ThreadIntersection& intersection) const;
    WebShapeCache& GetOrCreateWebShape(
        uint64_t key,
        size_t spokeCount,
        size_t ringCount);

private:
    Camera* camera_ = nullptr;
    std::unique_ptr<ThreadRenderer> renderer_;

    std::vector<std::vector<PhysicsNode>> webNodes_;
    std::unordered_map<uint64_t, WebShapeCache> webShapeCache_;

    float spokeLength_ = 1.5f;
    float innerHoleRadius_ = 0.03f;
    float liftY_ = 0.02f;

    std::array<float, 5> ringRates_ = {0.18f, 0.34f, 0.52f, 0.73f, 0.96f};

    float webThickness_ = 0.008f;
    int radialSegments_ = 4;
    int maxIntersections_ = 64;

    int arcSegments_ = 4;        // 輪糸1スパンあたりの分割数（増やすと滑らかになります）
    float arcSagFactor_ = 0.15f; // たるみ具合の係数（スパン長に対する割合）

    float missingThreadProbability_ = 0.15f; // 糸が張られない（欠損する）確率
};