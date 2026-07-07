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
    renderer_->Initialize(maxThreads, nodesPerThread, kThreadThickness, kThreadDrawSegments);
}

/// <summary>
/// 更新
/// </summary>
void ThreadManager::Update() {
    // 1. 交差判定の更新
    CalculateIntersections();

    // =========================================================
    // ★ ここで「普段の糸の材質（張り具合）」を設定する！
    // =========================================================
    for (auto& physics : physicsList_) {
        physics->SetIterations(kDefaultIterations);
        physics->SetStiffness(kDefaultStiffness);
        physics->SetSlack(kDefaultSlack);
    }

    // =========================================================
    // ★ 交差している糸は「より強靭に」設定する
    // =========================================================
    for (const auto& inter : intersections_) {
        physicsList_[inter.threadIndexA]->SetIterations(kIntersectedIterations);
        physicsList_[inter.threadIndexB]->SetIterations(kIntersectedIterations);

        physicsList_[inter.threadIndexA]->SetStiffness(kIntersectedStiffness);
        physicsList_[inter.threadIndexB]->SetStiffness(kIntersectedStiffness);
    }

    // 3. 物理演算の実行
    std::vector<std::vector<PhysicsNode>> allNodes;
    UpdatePhysics(allNodes);

    // 描画用の情報を更新
    renderer_->Update(allNodes, camera_);
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

    Vector3 toClosest = {
        query.closestPoint.x - nextPos.x,
        0.0f,
        query.closestPoint.z - nextPos.z
    };

    float alongError = toClosest.x * tangent.x + toClosest.z * tangent.z;

    Vector3 lateral = {
        toClosest.x - tangent.x * alongError,
        0.0f,
        toClosest.z - tangent.z * alongError
    };

    float edgeFade = 1.0f;
    if (query.t < kThreadEndSnapFadeRange) {
        edgeFade = query.t / kThreadEndSnapFadeRange;
    } else if (query.t > (1.0f - kThreadEndSnapFadeRange)) {
        edgeFade = (1.0f - query.t) / kThreadEndSnapFadeRange;
    }
    edgeFade = std::clamp(edgeFade, 0.0f, 1.0f);

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
    float bestDistSq = radiusSq + kDistancePadding;

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        for (size_t i = 0; i + 1 < nodes.size(); ++i) {
            const Vector3 a = nodes[i].currentPos;
            const Vector3 b = nodes[i + 1].currentPos;

            const Vector3 flatA = FlattenXZ(a);
            const Vector3 flatB = FlattenXZ(b);

            const Vector3 ab = flatB - flatA;
            const Vector3 ap = flatPos - flatA;

            const float abLenSq = ab.x * ab.x + ab.z * ab.z;

            float localT = 0.0f;
            if (abLenSq > 0.0f) {
                localT = Clamp01((ap.x * ab.x + ap.z * ab.z) / abLenSq);
            }

            Vector3 closestFlat = {
                flatA.x + ab.x * localT,
                0.0f,
                flatA.z + ab.z * localT
            };

            const float dx = flatPos.x - closestFlat.x;
            const float dz = flatPos.z - closestFlat.z;
            const float distSq = dx * dx + dz * dz;

            if (distSq <= radiusSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                found = true;

                const float closestY = a.y + (b.y - a.y) * localT;

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

bool ThreadManager::FindTargetThread(const Vector3& pos, const Vector3& moveDir, float radius, ThreadQueryResult& outResult) const {
    if (radius <= 0.0f) return false;

    const float radiusSq = radius * radius;
    const Vector3 flatPos = FlattenXZ(pos);

    Vector3 normMove = {moveDir.x, 0.0f, moveDir.z};
    float moveLen = std::sqrt(normMove.x * normMove.x + normMove.z * normMove.z);
    if (moveLen > kMinVectorLength) {
        normMove.x /= moveLen;
        normMove.z /= moveLen;
    } else {
        return false;
    }

    bool found = false;
    float bestScore = kInitialBestScore;

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        for (size_t i = 0; i + 1 < nodes.size(); ++i) {
            const Vector3 a = nodes[i].currentPos;
            const Vector3 b = nodes[i + 1].currentPos;
            const Vector3 flatA = FlattenXZ(a);
            const Vector3 flatB = FlattenXZ(b);
            const Vector3 ab = flatB - flatA;
            const Vector3 ap = flatPos - flatA;

            const float abLenSq = ab.x * ab.x + ab.z * ab.z;
            float localT = 0.0f;
            if (abLenSq > 0.0f) {
                localT = Clamp01((ap.x * ab.x + ap.z * ab.z) / abLenSq);
            }

            Vector3 closestFlat = {flatA.x + ab.x * localT, 0.0f, flatA.z + ab.z * localT};
            const float dx = closestFlat.x - flatPos.x;
            const float dz = closestFlat.z - flatPos.z;
            const float distSq = dx * dx + dz * dz;

            if (distSq <= radiusSq) {
                float dist = std::sqrt(distSq);

                Vector3 toThread = {dx, 0.0f, dz};
                float posDot = 1.0f;
                if (dist > kMinDistForDirection) {
                    toThread.x /= dist;
                    toThread.z /= dist;
                    posDot = toThread.x * normMove.x + toThread.z * normMove.z;
                }

                if (posDot < kTargetFrontAngleThreshold)
                {
                    continue;
                }

                float threadLen = std::sqrt(abLenSq);
                float dirDot = 0.0f;
                if (threadLen > kMinVectorLength) {
                    float tDirX = ab.x / threadLen;
                    float tDirZ = ab.z / threadLen;
                    dirDot = std::abs(tDirX * normMove.x + tDirZ * normMove.z);
                }

                if (dirDot < kTargetParallelAngleThreshold) {
                    continue;
                }

                float distScore = 1.0f - (dist / radius);
                float frontScore = std::max(0.0f, posDot);
                float parallelScore = dirDot;

                float score = (distScore * kWeightDistance) + (frontScore * kWeightFront) + (parallelScore * kWeightParallel);

                if (score > bestScore) {
                    bestScore = score;
                    found = true;

                    const float closestY = a.y + (b.y - a.y) * localT;
                    outResult.closestPoint = {closestFlat.x, closestY, closestFlat.z};
                    outResult.startPoint = nodes.front().currentPos;
                    outResult.endPoint = nodes.back().currentPos;
                    outResult.segmentStart = a;
                    outResult.segmentEnd = b;
                    outResult.t = (static_cast<float>(i) + localT) / static_cast<float>(nodes.size() - 1);
                }
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

bool ThreadManager::CanCreateThread(const Vector3& startPos, const Vector3& endPos, float minDistance) const
{
    if (physicsList_.empty()) return true;

    // 新しい糸の方向ベクトルを計算
    Vector3 newDir = {endPos.x - startPos.x, 0.0f, endPos.z - startPos.z};
    float newLenSq = newDir.x * newDir.x + newDir.z * newDir.z;
    if (newLenSq < 0.0001f) return false;

    float newLen = std::sqrt(newLenSq);
    newDir.x /= newLen;
    newDir.z /= newLen;

    // 距離をチェックするポイント（始点、中間点、終点）
    Vector3 points[3] = {
        startPos,
        {(startPos.x + endPos.x) * 0.5f, startPos.y, (startPos.z + endPos.z) * 0.5f},
        endPos
    };

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        // 既存の糸の全体方向
        Vector3 oldStart = nodes.front().currentPos;
        Vector3 oldEnd = nodes.back().currentPos;

        Vector3 oldDir = {oldEnd.x - oldStart.x, 0.0f, oldEnd.z - oldStart.z};
        float oldLen = std::sqrt(oldDir.x * oldDir.x + oldDir.z * oldDir.z);

        if (oldLen > 0.0001f) {
            oldDir.x /= oldLen;
            oldDir.z /= oldLen;

            // 1. 平行度のチェック（内積の絶対値が 1.0 に近いほど平行）
            float dot = std::abs(newDir.x * oldDir.x + newDir.z * oldDir.z);

            // 交差するような角度（X字など）は許容する。
            // 0.85f は約31度。これより平行に近い（角度が浅い）場合のみ距離チェックを行う
            if (dot > 0.85f) {
                // 2. 距離のチェック
                for (const auto& pt : points) {
                    float dx = oldEnd.x - oldStart.x;
                    float dz = oldEnd.z - oldStart.z;
                    float lenSq = dx * dx + dz * dz;

                    float t = 0.0f;
                    if (lenSq > 0.0001f) {
                        float px = pt.x - oldStart.x;
                        float pz = pt.z - oldStart.z;
                        t = std::clamp((px * dx + pz * dz) / lenSq, 0.0f, 1.0f);
                    }

                    // 既存の糸の線分上で最も近い点
                    float closestX = oldStart.x + t * dx;
                    float closestZ = oldStart.z + t * dz;

                    float distSq = (pt.x - closestX) * (pt.x - closestX) +
                        (pt.z - closestZ) * (pt.z - closestZ);

                    // 指定した距離より近ければ生成不可
                    if (distSq < minDistance * minDistance) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

/// <summary>
/// 進行方向を考慮して、最も移動に適した糸を検索する（交差点用）
/// </summary>
bool ThreadManager::FindBestThread(const Vector3& pos, const Vector3& moveDir, float radius, ThreadQueryResult& outResult) const {
    if (radius <= 0.0f) {
        return false;
    }

    const float radiusSq = radius * radius;
    const Vector3 flatPos = FlattenXZ(pos);

    bool found = false;
    float bestScore = kInitialBestScore;
    ThreadQueryResult bestQuery {};

    Vector3 normalizedMoveDir = {moveDir.x, 0.0f, moveDir.z};
    float moveLen = std::sqrt(normalizedMoveDir.x * normalizedMoveDir.x + normalizedMoveDir.z * normalizedMoveDir.z);
    if (moveLen > 0.0f) {
        normalizedMoveDir.x /= moveLen;
        normalizedMoveDir.z /= moveLen;
    }

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        for (size_t i = 0; i + 1 < nodes.size(); ++i) {
            const Vector3 a = nodes[i].currentPos;
            const Vector3 b = nodes[i + 1].currentPos;

            const Vector3 flatA = FlattenXZ(a);
            const Vector3 flatB = FlattenXZ(b);

            const Vector3 ab = flatB - flatA;
            const Vector3 ap = flatPos - flatA;

            const float abLenSq = ab.x * ab.x + ab.z * ab.z;

            float localT = 0.0f;
            if (abLenSq > 0.0f) {
                localT = Clamp01((ap.x * ab.x + ap.z * ab.z) / abLenSq);
            }

            Vector3 closestFlat = {
                flatA.x + ab.x * localT,
                0.0f,
                flatA.z + ab.z * localT
            };

            const float dx = flatPos.x - closestFlat.x;
            const float dz = flatPos.z - closestFlat.z;
            const float distSq = dx * dx + dz * dz;

            if (distSq <= radiusSq) {
                float dist = std::sqrt(distSq);

                Vector3 threadDir = {b.x - a.x, 0.0f, b.z - a.z};
                float tLen = std::sqrt(threadDir.x * threadDir.x + threadDir.z * threadDir.z);
                if (tLen > 0.0f) {
                    threadDir.x /= tLen;
                    threadDir.z /= tLen;
                }

                float distScore = 1.0f - (dist / radius);
                float score = 0.0f;

                if (moveLen > 0.0f) {
                    float dot = std::abs(normalizedMoveDir.x * threadDir.x + normalizedMoveDir.z * threadDir.z);
                    score = distScore + (dot * kMoveDirectionBonus);
                } else {
                    score = distScore;
                }

                if (score > bestScore) {
                    bestScore = score;
                    found = true;

                    const float closestY = a.y + (b.y - a.y) * localT;

                    bestQuery.closestPoint = {closestFlat.x, closestY, closestFlat.z};
                    bestQuery.startPoint = nodes.front().currentPos;
                    bestQuery.endPoint = nodes.back().currentPos;
                    bestQuery.segmentStart = a;
                    bestQuery.segmentEnd = b;
                    bestQuery.t = (static_cast<float>(i) + localT) / static_cast<float>(nodes.size() - 1);
                }
            }
        }
    }

    if (found) {
        outResult = bestQuery;
    }
    return found;
}

// ---------------------------------------------------------
// 内部処理
// ---------------------------------------------------------
void ThreadManager::UpdatePhysics(std::vector<std::vector<PhysicsNode>>& outAllNodes) {
    outAllNodes.clear();

    for (const auto& inter : intersections_) {
        auto& nodesA = physicsList_[inter.threadIndexA]->GetNodesMutable();
        auto& nodesB = physicsList_[inter.threadIndexB]->GetNodesMutable();

        PhysicsNode& nA0 = nodesA[inter.segmentIndexA];
        PhysicsNode& nA1 = nodesA[inter.segmentIndexA + 1];

        PhysicsNode& nB0 = nodesB[inter.segmentIndexB];
        PhysicsNode& nB1 = nodesB[inter.segmentIndexB + 1];

        Vector3 centerA = (nA0.currentPos + nA1.currentPos) * 0.5f;
        Vector3 centerB = (nB0.currentPos + nB1.currentPos) * 0.5f;

        Vector3 diff = (centerB - centerA) * 0.5f * kIntersectionPullStiffness;

        if (nA0.mass > 0.0f) nA0.currentPos += diff;
        if (nA1.mass > 0.0f) nA1.currentPos += diff;

        if (nB0.mass > 0.0f) nB0.currentPos -= diff;
        if (nB1.mass > 0.0f) nB1.currentPos -= diff;
    }

    for (auto& physics : physicsList_) {
        physics->Update();
    }

    for (auto& physics : physicsList_) {
        outAllNodes.push_back(physics->GetNodes());
    }

    // 全ての交差点について、互いの糸を引っ張り合わせる（連動してたわませる）
    for (const auto& intersection : intersections_) {
        // 交差している2つのThreadPhysicsを取得
        auto& physicsA = physicsList_[intersection.threadIndexA];
        auto& physicsB = physicsList_[intersection.threadIndexB];

        // 交差している線分の始点ノードのインデックス
        int nodeIndexA = static_cast<int>(intersection.segmentIndexA);
        int nodeIndexB = static_cast<int>(intersection.segmentIndexB);

        // 現在のノードの座標を取得
        Vector3 posA = physicsA->GetNodePosition(nodeIndexA);
        Vector3 posB = physicsB->GetNodePosition(nodeIndexB);

        // 2つのノードの距離差分（AからBへ向かうベクトル）
        Vector3 diff = posB - posA;

        // 引っ張り合う力の計算（kIntersectionPullStiffness を掛ける）
        // ※ Y軸のたわみだけを連動させたい場合は、diff.x と diff.z を 0.0f にしても良いです
        Vector3 pullForce = diff * kIntersectionPullStiffness;

        //// お互いを引き寄せるように位置を補正する（質量が同じと仮定して半分ずつ動かす）
        //physicsA->AddPositionOffset(nodeIndexA, pullForce * 0.5f);
        //physicsB->AddPositionOffset(nodeIndexB, pullForce * -0.5f);

        // =========================================================
        // ノードが壁に固定されていない（mass > 0.0f）場合のみ動かす！
        // =========================================================
        if (physicsA->GetNodes()[nodeIndexA].mass > 0.0f) {
            physicsA->AddPositionOffset(nodeIndexA, pullForce * 0.5f);
        }
        if (physicsB->GetNodes()[nodeIndexB].mass > 0.0f) {
            physicsB->AddPositionOffset(nodeIndexB, pullForce * -0.5f);
        }
    }
}

void ThreadManager::CalculateIntersections() {
    intersections_.clear();

    if (physicsList_.size() < 2) {
        return;
    }

    for (size_t i = 0; i < physicsList_.size(); ++i) {
        const auto& nodesA = physicsList_[i]->GetNodes();
        if (nodesA.size() < 2) continue;

        for (size_t j = i + 1; j < physicsList_.size(); ++j) {
            const auto& nodesB = physicsList_[j]->GetNodes();
            if (nodesB.size() < 2) continue;

            for (size_t a = 0; a + 1 < nodesA.size(); ++a) {
                for (size_t b = 0; b + 1 < nodesB.size(); ++b) {

                    Vector3 intersectPos {};
                    if (!SegmentIntersectXZ(
                        nodesA[a].currentPos, nodesA[a + 1].currentPos,
                        nodesB[b].currentPos, nodesB[b + 1].currentPos,
                        intersectPos)) {
                        continue;
                    }

                    if (IsNearSameIntersection(intersections_, intersectPos, kSameIntersectionThreshold)) {
                        continue;
                    }

                    ThreadIntersection intersection {};
                    intersection.position = intersectPos;

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