#include "SpiderWebManager.h"

#include "Camera.h"
#include "MathFunction.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {

    float LengthXZ(const Vector3& v) {
        return std::sqrt(v.x * v.x + v.z * v.z);
    }

    Vector3 NormalizeXZ(const Vector3& v) {
        float length = LengthXZ(v);
        if (length <= 0.0001f) {
            return {0.0f, 0.0f, 0.0f};
        }
        return {v.x / length, 0.0f, v.z / length};
    }

    float AngleXZ(const Vector3& dir) {
        return std::atan2(dir.z, dir.x);
    }

    bool IsZeroXZ(const Vector3& v) {
        return std::abs(v.x) <= 0.0001f && std::abs(v.z) <= 0.0001f;
    }

    struct OrderedDirection {
        Vector3 dir;
        float angle = 0.0f;
    };

} // namespace

float SpiderWebManager::Hash01(float x) const {
    float s = std::sin(x * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

uint64_t SpiderWebManager::MakeIntersectionKey(
    const ThreadManager::ThreadIntersection& intersection) const {

    uint64_t key = 1469598103934665603ULL;

    auto mix = [&key](uint64_t v) {
        key ^= v;
        key *= 1099511628211ULL;
        };

    mix(static_cast<uint64_t>(intersection.threadIndexA));
    mix(static_cast<uint64_t>(intersection.threadIndexB));
    mix(static_cast<uint64_t>(intersection.segmentIndexA));
    mix(static_cast<uint64_t>(intersection.segmentIndexB));

    return key;
}

SpiderWebManager::WebShapeCache& SpiderWebManager::GetOrCreateWebShape(
    uint64_t key,
    size_t spokeCount,
    size_t ringCount) {

    auto& cache = webShapeCache_[key];
    if (cache.initialized) {
        return cache;
    }

    cache.spokeLengthRates.resize(spokeCount, 1.0f);
    cache.ringRadiusRates.resize(spokeCount * ringCount, 1.0f);

    const float baseSeed = static_cast<float>(key & 0xFFFFFFFFULL);

    for (size_t i = 0; i < spokeCount; ++i) {
        float jitter = 0.85f + Hash01(baseSeed + static_cast<float>(i) * 7.13f) * 0.25f;
        cache.spokeLengthRates[i] = jitter;
    }

    for (size_t r = 0; r < ringCount; ++r) {
        for (size_t i = 0; i < spokeCount; ++i) {
            size_t index = r * spokeCount + i;
            float jitter = 0.93f + Hash01(
                baseSeed + static_cast<float>(r) * 13.7f + static_cast<float>(i) * 2.9f) * 0.14f;
            cache.ringRadiusRates[index] = jitter;
        }
    }

    cache.initialized = true;
    return cache;
}

void SpiderWebManager::Initialize(Camera* camera) {
    camera_ = camera;

    renderer_ = std::make_unique<ThreadRenderer>();

    const int maxSegmentsPerIntersection = 48;
    const int maxWebSegments = maxIntersections_ * maxSegmentsPerIntersection;

    // 蜘蛛糸1本は 2ノード構成
    renderer_->Initialize(maxWebSegments, 2, webThickness_, radialSegments_);
}

void SpiderWebManager::Update(const ThreadManager& threadManager) {
    webNodes_.clear();

    const auto& intersections = threadManager.GetIntersections();
    for (const auto& intersection : intersections) {
        BuildWebFromIntersection(intersection);
    }

    if (renderer_) {
        renderer_->Update(webNodes_, camera_);
    }
}

void SpiderWebManager::BuildWebFromIntersection(
    const ThreadManager::ThreadIntersection& intersection) {

    Vector3 center = intersection.position;
    center.y += liftY_;

    std::array<Vector3, 4> baseDirs = {
        NormalizeXZ(intersection.segmentAStart - intersection.position),
        NormalizeXZ(intersection.segmentAEnd - intersection.position),
        NormalizeXZ(intersection.segmentBStart - intersection.position),
        NormalizeXZ(intersection.segmentBEnd - intersection.position)
    };

    std::vector<OrderedDirection> ordered;
    ordered.reserve(4);

    for (const auto& dir : baseDirs) {
        if (IsZeroXZ(dir)) {
            continue;
        }
        ordered.push_back({dir, AngleXZ(dir)});
    }

    if (ordered.size() < 4) {
        return;
    }

    std::sort(ordered.begin(), ordered.end(),
        [](const OrderedDirection& a, const OrderedDirection& b) {
            return a.angle < b.angle;
        });

    // 4方向 + 中間4方向 = 8本の spoke
    std::vector<Vector3> spokeDirs;
    spokeDirs.reserve(8);

    for (size_t i = 0; i < 4; ++i) {
        const Vector3 d0 = ordered[i].dir;
        const Vector3 d1 = ordered[(i + 1) % 4].dir;

        spokeDirs.push_back(d0);

        Vector3 mid = NormalizeXZ(d0 + d1);
        if (!IsZeroXZ(mid)) {
            spokeDirs.push_back(mid);
        }
    }

    if (spokeDirs.size() < 6) {
        return;
    }

    const uint64_t key = MakeIntersectionKey(intersection);
    WebShapeCache& shapeCache =
        GetOrCreateWebShape(key, spokeDirs.size(), ringRates_.size());

    std::vector<float> spokeLengths(spokeDirs.size(), spokeLength_);
    for (size_t i = 0; i < spokeDirs.size(); ++i) {
        spokeLengths[i] = spokeLength_ * shapeCache.spokeLengthRates[i];
    }

    // 放射糸
    for (size_t i = 0; i < spokeDirs.size(); ++i) {
        Vector3 start = center + spokeDirs[i] * innerHoleRadius_;
        Vector3 end = center + spokeDirs[i] * spokeLengths[i];
        start.y = center.y;
        end.y = center.y;
        AddSegmentAsThread(start, end);
    }

    // 輪糸
    for (size_t r = 0; r < ringRates_.size(); ++r) {
        std::vector<Vector3> ringPoints(spokeDirs.size());

        for (size_t i = 0; i < spokeDirs.size(); ++i) {
            size_t cacheIndex = r * spokeDirs.size() + i;
            float localJitter = shapeCache.ringRadiusRates[cacheIndex];
            float radius = spokeLengths[i] * ringRates_[r] * localJitter;

            ringPoints[i] = center + spokeDirs[i] * radius;
            ringPoints[i].y = center.y;
        }

        for (size_t i = 0; i < ringPoints.size(); ++i) {
            AddSegmentAsThread(ringPoints[i], ringPoints[(i + 1) % ringPoints.size()]);
        }
    }
}

void SpiderWebManager::AddSegmentAsThread(const Vector3& start, const Vector3& end) {
    PhysicsNode n0 {};
    n0.currentPos = start;
    n0.previousPos = start;
    n0.mass = 0.0f;

    PhysicsNode n1 {};
    n1.currentPos = end;
    n1.previousPos = end;
    n1.mass = 0.0f;

    webNodes_.push_back({n0, n1});
}

void SpiderWebManager::Draw() const {
    if (!renderer_) {
        return;
    }
    renderer_->Draw();
}