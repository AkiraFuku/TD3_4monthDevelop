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

    // ▼ 修正箇所: 初期化済みでも、放射糸の本数が変わっていたら再構築（リサイズ）を許可する
    if (cache.initialized && cache.spokeLengthRates.size() == spokeCount) {
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

    // 輪糸を arcSegments_ 分割するため、交差点あたりのセグメント数を増やす
    // (例: 放射糸8本 + 輪糸40スパン × 4分割 = 168セグメント。余裕を見て256に設定)
    const int maxSegmentsPerIntersection = 256;
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
    spokeDirs.reserve(16); // 余裕を持たせる

    const float pi = 3.14159265f;
    const float targetAngleDiff = 30.0f * (pi / 180.0f); // 例: 約30度ごとに分割

    for (size_t i = 0; i < 4; ++i) {
        const OrderedDirection& o0 = ordered[i];
        const OrderedDirection& o1 = ordered[(i + 1) % 4];

        spokeDirs.push_back(o0.dir);

        // 角度の差を計算（-π 〜 π を跨ぐラップアラウンドを考慮）
        float a0 = o0.angle;
        float a1 = o1.angle;
        if (a1 <= a0) {
            a1 += 2.0f * PI;
        }
        float diff = a1 - a0;

        // 目標角度で分割（隙間が広ければ分割数が増える）
        int splits = std::max(1, static_cast<int>(std::round(diff / targetAngleDiff)));

        for (int s = 1; s < splits; ++s) {
            float t = static_cast<float>(s) / splits;
            float midAngle = a0 + diff * t;
            spokeDirs.push_back({std::cos(midAngle), 0.0f, std::sin(midAngle)});
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
            Vector3 start = ringPoints[i];
            Vector3 end = ringPoints[(i + 1) % ringPoints.size()];

            // --- 弧を描くように分割して追加 ---
            Vector3 mid = (start + end) * 0.5f;
            Vector3 sagDir = NormalizeXZ(center - mid); // 中心に向かう方向ベクトル

            // 始点と終点の距離に基づいて、たるみの最大量を決定
            float spanLength = LengthXZ(end - start);
            float maxSag = spanLength * arcSagFactor_;

            Vector3 prevPos = start;
            for (int j = 1; j <= arcSegments_; ++j) {
                float t = static_cast<float>(j) / arcSegments_;

                // 1. 直線上の基本位置
                Vector3 basePos = start + (end - start) * t;

                // 2. 放物線状のたるみを計算 (t=0.5 のとき parabola=1.0 になる)
                float parabola = 4.0f * t * (1.0f - t);

                // 3. 基本位置から中心に向かってたるませる
                Vector3 currentPos = basePos + sagDir * (maxSag * parabola);
                currentPos.y = center.y; // 高さは揃える

                AddSegmentAsThread(prevPos, currentPos);
                prevPos = currentPos;
            }
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