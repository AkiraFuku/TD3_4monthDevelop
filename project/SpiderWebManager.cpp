#include "SpiderWebManager.h"

#include "Camera.h"
#include "MathFunction.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// ============================================================
// ファイルローカルユーティリティ
// ============================================================
namespace {

    float LengthXZ(const Vector3& v) {
        return std::sqrt(v.x * v.x + v.z * v.z);
    }

    // XZ 平面上の正規化（ゼロベクトルの場合はゼロを返す）
    Vector3 NormalizeXZ(const Vector3& v) {
        float len = LengthXZ(v);
        if (len <= 0.0001f) return {0.0f, 0.0f, 0.0f};
        return {v.x / len, 0.0f, v.z / len};
    }

    // 3D 空間上の正規化
    Vector3 Normalize3D(const Vector3& v) {
        float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len <= 0.0001f) return {0.0f, 0.0f, 0.0f};
        return {v.x / len, v.y / len, v.z / len};
    }

    // XY 平面上の正規化（BuildWebStandaloneXY 専用）
    Vector3 NormalizeXY(const Vector3& v) {
        float len = std::sqrt(v.x * v.x + v.y * v.y);
        if (len <= 0.0001f) return {0.0f, 0.0f, 0.0f};
        return {v.x / len, v.y / len, 0.0f};
    }

    float AngleXZ(const Vector3& dir) {
        return std::atan2(dir.z, dir.x);
    }

    bool IsZeroXZ(const Vector3& v) {
        return std::abs(v.x) <= 0.0001f && std::abs(v.z) <= 0.0001f;
    }

    // イーズアウト（2次）
    float EaseOut(float t) {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    struct OrderedDirection {
        Vector3 dir;
        float   angle = 0.0f;
    };

} // namespace

// ============================================================
// ハッシュ・キー生成
// ============================================================

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

// ============================================================
// キャッシュ管理
// ============================================================

SpiderWebManager::WebShapeCache& SpiderWebManager::GetOrCreateWebShape(
    uint64_t key, size_t spokeCount, size_t ringCount) {

    auto& cache = webShapeCache_[key];

    // スポーク数が変化したときのみ再初期化
    if (cache.initialized && cache.spokeLengthRates.size() == spokeCount) {
        return cache;
    }

    cache.spokeLengthRates.resize(spokeCount, 1.0f);
    cache.ringRadiusRates.resize(spokeCount * ringCount, 1.0f);

    const float baseSeed = static_cast<float>(key & 0xFFFFFFFFULL);

    for (size_t i = 0; i < spokeCount; ++i) {
        cache.spokeLengthRates[i] =
            0.85f + Hash01(baseSeed + static_cast<float>(i) * 7.13f) * 0.25f;
    }
    for (size_t r = 0; r < ringCount; ++r) {
        for (size_t i = 0; i < spokeCount; ++i) {
            cache.ringRadiusRates[r * spokeCount + i] =
                0.93f + Hash01(baseSeed
                               + static_cast<float>(r) * 13.7f
                               + static_cast<float>(i) * 2.9f) * 0.14f;
        }
    }

    // ワークバッファも同時に確保（以後は resize されない）
    cache.workSpokeLengths.resize(spokeCount);
    cache.workRingPoints.resize(spokeCount);

    cache.initialized = true;
    return cache;
}

// ============================================================
// 輪糸スパン共通ヘルパー
// ============================================================

void SpiderWebManager::AddRingSpanArc(
    const Vector3& start, const Vector3& end,
    const Vector3& center, float maxSag) {

    const Vector3 diff = end - start;

    // たるみ方向: 中点→中心（XZ 平面内）を正規化
    // ※ BuildWebStandaloneXY（XY 平面）から呼ぶ場合は呼び出し前に Y を揃えること
    const Vector3 mid = (start + end) * 0.5f;
    const Vector3 sagDir = NormalizeXZ(center - mid);

    Vector3 prevPos = start;
    for (int j = 1; j <= arcSegments_; ++j) {
        const float t_arc = static_cast<float>(j) / static_cast<float>(arcSegments_);
        const float parabola = 4.0f * t_arc * (1.0f - t_arc);
        const Vector3 curPos = start + diff * t_arc + sagDir * (maxSag * parabola);
        AddSegmentAsThread(prevPos, curPos);
        prevPos = curPos;
    }
}

// ============================================================
// 初期化 / 更新 / 描画
// ============================================================

void SpiderWebManager::Initialize(Camera* camera) {
    camera_ = camera;
    renderer_ = std::make_unique<ThreadRenderer>();

    // 放射糸8本 + 輪糸 (5リング × 8スパン × 4分割) = 168 セグメント。余裕を見て 256。
    const int maxSegmentsPerIntersection = 256;
    const int maxWebSegments = maxIntersections_ * maxSegmentsPerIntersection;
    renderer_->Initialize(maxWebSegments, 2, webThickness_, radialSegments_);

    renderer_->SetColor(webColor_);
}

void SpiderWebManager::Update(const ThreadManager& threadManager) {
    webNodes_.clear();

    for (const auto& intersection : threadManager.GetIntersections()) {
        BuildWebFromIntersection(intersection);
    }

    if (renderer_) {
        renderer_->Update(webNodes_, camera_);
    }
}

void SpiderWebManager::Draw() const {
    if (renderer_) {
        renderer_->Draw();
    }
}

// ============================================================
// スタンドアロン巣（タイトル画面など）
// ============================================================

void SpiderWebManager::UpdateStandaloneXY(
    const Vector3& center, float scale, const Vector3& rotation, float progress) {

    webNodes_.clear();
    BuildWebStandaloneXY(center, scale, rotation, progress, 9999ULL);

    if (renderer_) {
        renderer_->Update(webNodes_, camera_);
    }
}

void SpiderWebManager::SetWebColor(const Vector4& color) {
    webColor_ = color;
    if (renderer_) {
        renderer_->SetColor(webColor_);
    }
}

void SpiderWebManager::BuildWebStandaloneXY(
    const Vector3& center, float scale, const Vector3& rotation,
    float progress, uint64_t key) {

    // 8方向の基底スポーク
    constexpr std::array<Vector3, 8> kBaseDirs = {{
        {1.0f, 0.0f, 0.0f}, {0.7071f, 0.7071f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {-0.7071f, 0.7071f, 0.0f},
        {-1.0f, 0.0f, 0.0f}, {-0.7071f, -0.7071f, 0.0f},
        {0.0f, -1.0f, 0.0f}, {0.7071f, -0.7071f, 0.0f},
        }};

    // --- 3D回転の適用 ---
    float cx = std::cos(rotation.x), sx = std::sin(rotation.x);
    float cy = std::cos(rotation.y), sy = std::sin(rotation.y);
    float cz = std::cos(rotation.z), sz = std::sin(rotation.z);

    std::array<Vector3, 8> spokeDirs;
    for (size_t i = 0; i < kBaseDirs.size(); ++i) {
        float x = kBaseDirs[i].x;
        float y = kBaseDirs[i].y;
        float z = 0.0f;

        // Z軸回転
        float x1 = x * cz - y * sz;
        float y1 = x * sz + y * cz;
        float z1 = z;
        // X軸回転
        float x2 = x1;
        float y2 = y1 * cx - z1 * sx;
        float z2 = y1 * sx + z1 * cx;
        // Y軸回転
        float x3 = x2 * cy + z2 * sy;
        float y3 = y2;
        float z3 = -x2 * sy + z2 * cy;

        spokeDirs[i] = {x3, y3, z3};
    }

    const size_t spokeCount = spokeDirs.size();
    WebShapeCache& cache = GetOrCreateWebShape(key, spokeCount, ringRates_.size());

    const float easeProgress = EaseOut(progress);

    // 放射糸の長さを計算
    for (size_t i = 0; i < spokeCount; ++i) {
        cache.workSpokeLengths[i] = spokeLength_ * scale * cache.spokeLengthRates[i];
    }

    // 放射糸の構築
    for (size_t i = 0; i < spokeCount; ++i) {
        float currentLength = cache.workSpokeLengths[i] * easeProgress;
        AddSegmentAsThread(center, center + spokeDirs[i] * currentLength);
    }

    // 輪糸の構築
    const float ringCountF = static_cast<float>(ringRates_.size());
    for (size_t r = 0; r < ringRates_.size(); ++r) {
        const float ringStartThreshold = (static_cast<float>(r) / ringCountF) * 0.4f;
        if (progress <= ringStartThreshold) continue;

        const float localProgress =
            std::clamp((progress - ringStartThreshold) / (1.0f - ringStartThreshold), 0.0f, 1.0f);
        const float localEase = EaseOut(localProgress);

        for (size_t i = 0; i < spokeCount; ++i) {
            const float radius =
                cache.workSpokeLengths[i] * ringRates_[r]
                * cache.ringRadiusRates[r * spokeCount + i]
                * localEase;
            cache.workRingPoints[i] = center + spokeDirs[i] * radius;
        }

        const float maxSagBase = arcSagFactor_ * localEase;
        for (size_t i = 0; i < spokeCount; ++i) {
            const Vector3& start = cache.workRingPoints[i];
            const Vector3& end = cache.workRingPoints[(i + 1) % spokeCount];
            const Vector3  diff = end - start;

            const Vector3 mid = (start + end) * 0.5f;
            // ★ たるみ計算を 3D に変更
            const Vector3 sagDir = Normalize3D(center - mid);

            // ★ 距離計算も 3D に変更
            const float spanLength = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            const float maxSag = spanLength * maxSagBase;

            Vector3 prevPos = start;
            for (int j = 1; j <= arcSegments_; ++j) {
                const float t_arc = static_cast<float>(j) / static_cast<float>(arcSegments_);
                const float parabola = 4.0f * t_arc * (1.0f - t_arc);
                const Vector3 curPos = start + diff * t_arc + sagDir * (maxSag * parabola);
                AddSegmentAsThread(prevPos, curPos);
                prevPos = curPos;
            }
        }
    }
}

// ============================================================
// 交差点からの巣構築
// ============================================================

void SpiderWebManager::BuildWebFromIntersection(
    const ThreadManager::ThreadIntersection& intersection) {

    Vector3 center = intersection.position;
    center.y += liftY_;

    // 4方向を収集してゼロベクトルを除外
    const std::array<Vector3, 4> baseDirs = {{
            NormalizeXZ(intersection.segmentAStart - intersection.position),
            NormalizeXZ(intersection.segmentAEnd - intersection.position),
            NormalizeXZ(intersection.segmentBStart - intersection.position),
            NormalizeXZ(intersection.segmentBEnd - intersection.position),
        }};

    std::array<OrderedDirection, 4> ordered;
    int validCount = 0;
    for (const auto& dir : baseDirs) {
        if (!IsZeroXZ(dir)) {
            ordered[validCount++] = {dir, AngleXZ(dir)};
        }
    }
    if (validCount < 4) return;

    std::sort(ordered.begin(), ordered.end(),
              [](const OrderedDirection& a, const OrderedDirection& b) {
                  return a.angle < b.angle;
              });

    // 4方向の間を細分割して spokeDirs を構築
    // 最大16本を見越してスタック確保（ほぼ常にスタックで収まる）
    std::array<Vector3, 16> spokeDirsStack;
    int spokeCountStack = 0;

    constexpr float kTargetAngleDiff = 30.0f * (3.14159265f / 180.0f);

    for (int i = 0; i < 4; ++i) {
        float a0 = ordered[i].angle;
        float a1 = ordered[(i + 1) % 4].angle;
        if (a1 <= a0) a1 += 2.0f * PI;
        const float diff = a1 - a0;

        if (spokeCountStack < static_cast<int>(spokeDirsStack.size())) {
            spokeDirsStack[spokeCountStack++] = ordered[i].dir;
        }

        const int splits = std::max(1, static_cast<int>(std::round(diff / kTargetAngleDiff)));
        for (int s = 1; s < splits; ++s) {
            if (spokeCountStack >= static_cast<int>(spokeDirsStack.size())) break;
            const float midAngle = a0 + diff * (static_cast<float>(s) / splits);
            spokeDirsStack[spokeCountStack++] = {std::cos(midAngle), 0.0f, std::sin(midAngle)};
        }
    }

    if (spokeCountStack < 6) return;

    const size_t spokeCount = static_cast<size_t>(spokeCountStack);
    const uint64_t key = MakeIntersectionKey(intersection);
    WebShapeCache& cache = GetOrCreateWebShape(key, spokeCount, ringRates_.size());

    // アニメーション進行
    if (cache.animationProgress < 1.0f) {
        cache.animationProgress = std::min(cache.animationProgress + 0.033f, 1.0f);
    }

    const float t = cache.animationProgress;
    const float easeProgress = EaseOut(t);

    // --- 放射糸の長さを計算してワークバッファへ書き込む ---
    for (size_t i = 0; i < spokeCount; ++i) {
        cache.workSpokeLengths[i] = spokeLength_ * cache.spokeLengthRates[i];
    }

    // 放射糸
    for (size_t i = 0; i < spokeCount; ++i) {
        const Vector3 start = {
            center.x + spokeDirsStack[i].x * innerHoleRadius_,
            center.y,
            center.z + spokeDirsStack[i].z * innerHoleRadius_,
        };
        const float currentLength = cache.workSpokeLengths[i] * easeProgress;
        const Vector3 end = {
            center.x + spokeDirsStack[i].x * currentLength,
            center.y,
            center.z + spokeDirsStack[i].z * currentLength,
        };
        AddSegmentAsThread(start, end);
    }

    // 輪糸（内側から順次展開）
    const float ringCountF = static_cast<float>(ringRates_.size());
    const float invRingCountF = 1.0f / ringCountF;
    for (size_t r = 0; r < ringRates_.size(); ++r) {
        const float ringStartThreshold = static_cast<float>(r) * invRingCountF;
        if (t <= ringStartThreshold) continue;

        const float localProgress =
            std::clamp((t - ringStartThreshold) * ringCountF, 0.0f, 1.0f);
        const float localEase = EaseOut(localProgress);

        // 輪糸頂点をワークバッファへ書き込む
        for (size_t i = 0; i < spokeCount; ++i) {
            const float radius =
                cache.workSpokeLengths[i] * ringRates_[r]
                * cache.ringRadiusRates[r * spokeCount + i]
                * localEase;
            cache.workRingPoints[i] = {
                center.x + spokeDirsStack[i].x * radius,
                center.y,
                center.z + spokeDirsStack[i].z * radius,
            };
        }

        // スパンごとに弧を追加
        for (size_t i = 0; i < spokeCount; ++i) {
            const Vector3& start = cache.workRingPoints[i];
            const Vector3& end = cache.workRingPoints[(i + 1) % spokeCount];
            const float spanLength = LengthXZ(end - start);
            const float maxSag = spanLength * arcSagFactor_ * localEase;
            AddRingSpanArc(start, end, center, maxSag);
        }
    }
}

// ============================================================
// プリミティブ追加
// ============================================================

void SpiderWebManager::AddSegmentAsThread(const Vector3& start, const Vector3& end) {
    PhysicsNode n0{};
    n0.currentPos = start;
    n0.previousPos = start;
    n0.mass = 0.0f;

    PhysicsNode n1{};
    n1.currentPos = end;
    n1.previousPos = end;
    n1.mass = 0.0f;

    webNodes_.push_back({n0, n1});
}