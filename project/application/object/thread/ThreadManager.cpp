#include "ThreadManager.h"

#include "Camera.h"
#include "DrawFunction.h"

#include <cassert>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------
// ローカル関数群 (無名名前空間)
// ---------------------------------------------------------
namespace {

    // Y軸を潰してXZ平面のみのベクトルにする
    Vector3 FlattenXZ(const Vector3& v) {
        return {v.x, 0.0f, v.z};
    }

    // 値を 0.0 ~ 1.0 の範囲に収める
    float Clamp01(float value) {
        return std::clamp(value, 0.0f, 1.0f);
    }

    // XZ平面での線分交差判定
    bool SegmentIntersectXZ(
        const Vector3& p0, const Vector3& p1,
        const Vector3& p2, const Vector3& p3,
        Vector3& outIntersect) {

        float s1_x = p1.x - p0.x;
        float s1_z = p1.z - p0.z;
        float s2_x = p3.x - p2.x;
        float s2_z = p3.z - p2.z;

        float det = (-s2_x * s1_z + s1_x * s2_z);
        // 平行な場合は交差しない
        if (std::abs(det) < 1e-6f) {
            return false;
        }

        float s = (-s1_z * (p0.x - p2.x) + s1_x * (p0.z - p2.z)) / det;
        float t = (s2_x * (p0.z - p2.z) - s2_z * (p0.x - p2.x)) / det;

        // 線分内に交差点があるか判定
        if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
            outIntersect.x = p0.x + (t * s1_x);
            outIntersect.y = p0.y + (t * (p1.y - p0.y)); // Y座標は線形補間
            outIntersect.z = p0.z + (t * s1_z);
            return true;
        }

        return false;
    }

    // すでに計算済みの交差点と近い位置にあるか判定
    bool IsNearSameIntersection(
        const std::vector<ThreadManager::ThreadIntersection>& intersections,
        const Vector3& pos,
        float threshold = 0.05f) {

        const float thresholdSq = threshold * threshold;

        for (const auto& intersection : intersections) {
            const float dx = intersection.position.x - pos.x;
            const float dz = intersection.position.z - pos.z;
            const float distSq = dx * dx + dz * dz;

            if (distSq <= thresholdSq) {
                return true;
            }
        }
        return false;
    }
}

// =========================================================
// ThreadManager クラス実装
// =========================================================

// ---------------------------------------------------------
// ライフサイクル
// ---------------------------------------------------------

/// <summary>
    /// 初期化
    /// </summary>
    /// <param name="maxThreads">生成可能なThreadの最大数</param>
    /// <param name="nodesPerThread">Threadのノード数</param>
    /// <param name="camera">Cameraのポインタ</param>
void ThreadManager::Initialize(int maxThreads, int nodesPerThread, Camera* camera) {
    assert(camera);
    camera_ = camera;
    maxThreads_ = maxThreads;
    nodesPerThread_ = nodesPerThread;

    physicsList_.reserve(maxThreads_);

    // 描画クラスの初期化
    renderer_ = std::make_unique<ThreadRenderer>();
    renderer_->Initialize(maxThreads, nodesPerThread, 0.02f, 6);
}

/// <summary>
/// 更新
/// </summary>
void ThreadManager::Update() {
    std::vector<std::vector<PhysicsNode>> allNodes;
    allNodes.reserve(physicsList_.size());

    // 物理挙動の更新とノード情報の収集
    UpdatePhysics(allNodes);

    // 描画用の情報を更新
    renderer_->Update(allNodes, camera_);

    // 糸同士の交差点計算
    CalculateIntersections();
}

/// <summary>
/// 描画
/// </summary>
void ThreadManager::Draw() {
    renderer_->Draw();
}

// ---------------------------------------------------------
// 糸の管理
// ---------------------------------------------------------

/// <summary>
/// Threadを生成
/// </summary>
/// <param name="startPos">Threadの始点</param>
/// <param name="endPos">Threadの終点</param>
void ThreadManager::AddThread(const Vector3& startPos, const Vector3& endPos) {
    // 最大数に達していたら追加しない
    if (physicsList_.size() >= maxThreads_) {
        return;
    }

    auto physics = std::make_unique<ThreadPhysics>();
    physics->Initialize(startPos, endPos, nodesPerThread_);
    physicsList_.push_back(std::move(physics));
}

/// <summary>
/// Threadをクリア
/// </summary>
void ThreadManager::ClearThreads() {
    physicsList_.clear();
    intersections_.clear();
}

ThreadManager::ConstrainedMoveResult ThreadManager::CalculateConstrainedVelocity(const Vector3& nextPos, const ThreadQueryResult& query) const {
    // 糸の接線ベクトル（方向）を計算
    Vector3 tangent = {
        query.endPoint.x - query.startPoint.x,
        0.0f,
        query.endPoint.z - query.startPoint.z
    };

    float tangentLength = std::sqrt(tangent.x * tangent.x + tangent.z * tangent.z);
    if (tangentLength > 0.0f) {
        tangent.x /= tangentLength;
        tangent.z /= tangentLength;
    }

    // 次の移動予定位置から、糸の最近点への差分ベクトル
    Vector3 toClosest = {
        query.closestPoint.x - nextPos.x,
        0.0f,
        query.closestPoint.z - nextPos.z
    };

    // 最近点への差を「接線方向成分」と「横方向成分」に分解
    float alongError = toClosest.x * tangent.x + toClosest.z * tangent.z;

    Vector3 lateral = {
        toClosest.x - tangent.x * alongError,
        0.0f,
        toClosest.z - tangent.z * alongError
    };

    // 端に近いほど横補正を弱める
    float edgeFade = 1.0f;
    if (query.t < kThreadEndSnapFadeRange) {
        edgeFade = query.t / kThreadEndSnapFadeRange;
    } else if (query.t > (1.0f - kThreadEndSnapFadeRange)) {
        edgeFade = (1.0f - query.t) / kThreadEndSnapFadeRange;
    }
    edgeFade = std::clamp(edgeFade, 0.0f, 1.0f);

    // 最終的な補正ベクトルを返す
    Vector3 correction = {
        lateral.x * kThreadLateralFollowStrength * edgeFade,
        0.0f,
        lateral.z * kThreadLateralFollowStrength * edgeFade
    };

    return {correction, edgeFade};
}

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
bool ThreadManager::FindNearestThread(const Vector3& pos, float radius, ThreadQueryResult& outResult) const {
    if (radius <= 0.0f) {
        return false;
    }

    const float radiusSq = radius * radius;
    const Vector3 flatPos = FlattenXZ(pos);

    bool found = false;
    float bestDistSq = radiusSq + 1.0f; // 判定半径より少し大きい値で初期化

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        // 糸を構成する各線分に対して判定
        for (size_t i = 0; i + 1 < nodes.size(); ++i) {
            const Vector3 a = nodes[i].currentPos;
            const Vector3 b = nodes[i + 1].currentPos;

            const Vector3 flatA = FlattenXZ(a);
            const Vector3 flatB = FlattenXZ(b);

            const Vector3 ab = flatB - flatA;
            const Vector3 ap = flatPos - flatA;

            const float abLenSq = ab.x * ab.x + ab.z * ab.z;

            // 線分AB上での最近接点の割合(t)を計算
            float localT = 0.0f;
            if (abLenSq > 0.0f) {
                localT = Clamp01((ap.x * ab.x + ap.z * ab.z) / abLenSq);
            }

            // XZ平面での最も近い座標
            Vector3 closestFlat = {
                flatA.x + ab.x * localT,
                0.0f,
                flatA.z + ab.z * localT
            };

            // 距離の二乗を計算して判定
            const float dx = flatPos.x - closestFlat.x;
            const float dz = flatPos.z - closestFlat.z;
            const float distSq = dx * dx + dz * dz;

            // より近く、かつ半径内であれば結果を更新
            if (distSq <= radiusSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                found = true;

                // Y座標(高さ)の計算
                const float closestY = a.y + (b.y - a.y) * localT;

                // 結果の格納
                outResult.closestPoint = {closestFlat.x, closestY, closestFlat.z};
                outResult.startPoint = nodes.front().currentPos;
                outResult.endPoint = nodes.back().currentPos;
                outResult.segmentStart = a;
                outResult.segmentEnd = b;
                outResult.t = (static_cast<float>(i) + localT) / static_cast<float>(nodes.size() - 1);
            }
        }
    }

    return found;
}

/// <summary>
/// 指定座標が糸の上にあるか判定
/// </summary>
/// <param name="pos">判定の基準となる座標</param>
/// <param name="radius">判定の許容範囲となる半径</param>
/// <returns>指定した範囲内に糸が存在すれば true、存在しなければ false</returns>
bool ThreadManager::IsOnThread(const Vector3& pos, float radius) const {
    ThreadQueryResult result {};
    return FindNearestThread(pos, radius, result);
}

/// <summary>
/// 指定座標から指定範囲内にある糸の高さ（Y座標）を取得する
/// </summary>
/// <param name="pos">検索の基準となる座標</param>
/// <param name="radius">検索対象とする判定半径</param>
/// <param name="outY">見つかった糸の高さ（Y座標）が格納される出力先</param>
/// <returns>範囲内に糸が見つかり、高さを取得できた場合は true、見つからなかった場合は false</returns>
bool ThreadManager::GetThreadHeight(const Vector3& pos, float radius, float& outY) const {
    ThreadQueryResult result {};

    // 糸が見つかった場合のみ高さを返す
    if (!FindNearestThread(pos, radius, result)) {
        return false;
    }

    outY = result.closestPoint.y;
    return true;
}

/// <summary>
/// 指定された範囲内の糸に対して、重さ（下方向への力）を適用する
/// </summary>
/// <param name="pos">重さを加える中心となる座標</param>
/// <param name="radius">重さが影響を及ぼす判定半径</param>
/// <param name="weight">適用する重さ（下方向への力の大きさ）</param>
void ThreadManager::ApplyWeight(const Vector3& pos, float radius, float weight) {
    Vector3 force = {0.0f, -weight, 0.0f};

    // 全ての糸に対してプレイヤーの重み(下方向への力)を適用
    for (auto& physics : physicsList_) {
        physics->ApplyForce(pos, radius, force);
    }
}

// ---------------------------------------------------------
// 内部処理
// ---------------------------------------------------------
void ThreadManager::UpdatePhysics(std::vector<std::vector<PhysicsNode>>& outAllNodes) {
    for (auto& physics : physicsList_) {
        physics->Update();
        outAllNodes.push_back(physics->GetNodes());
    }
}

void ThreadManager::CalculateIntersections() {
    intersections_.clear();

    // 糸が2本未満なら交差しない
    if (physicsList_.size() < 2) {
        return;
    }

    // 全ての糸の組み合わせをチェック
    for (size_t i = 0; i < physicsList_.size(); ++i) {
        const auto& nodesA = physicsList_[i]->GetNodes();
        if (nodesA.size() < 2) continue;

        for (size_t j = i + 1; j < physicsList_.size(); ++j) {
            const auto& nodesB = physicsList_[j]->GetNodes();
            if (nodesB.size() < 2) continue;

            // 糸Aと糸Bの全線分同士を判定
            for (size_t a = 0; a + 1 < nodesA.size(); ++a) {
                for (size_t b = 0; b + 1 < nodesB.size(); ++b) {

                    Vector3 intersectPos {};
                    // XZ平面で交差していなければスキップ
                    if (!SegmentIntersectXZ(
                        nodesA[a].currentPos, nodesA[a + 1].currentPos,
                        nodesB[b].currentPos, nodesB[b + 1].currentPos,
                        intersectPos)) {
                        continue;
                    }

                    // 既に近い位置に交差点が登録されていれば重複を避ける
                    if (IsNearSameIntersection(intersections_, intersectPos)) {
                        continue;
                    }

                    // 交差点情報の作成と登録
                    ThreadIntersection intersection {};
                    intersection.position = intersectPos;
                    // ※ヘッダ側で radius は 0.8f で初期化済み

                    intersection.threadIndexA = i;
                    intersection.threadIndexB = j;
                    intersection.segmentIndexA = a;
                    intersection.segmentIndexB = b;

                    intersection.segmentAStart = nodesA[a].currentPos;
                    intersection.segmentAEnd = nodesA[a + 1].currentPos;
                    intersection.segmentBStart = nodesB[b].currentPos;
                    intersection.segmentBEnd = nodesB[b + 1].currentPos;

                    intersections_.push_back(intersection);
                }
            }
        }
    }
}