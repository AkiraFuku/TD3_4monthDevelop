#define NOMINMAX
#include "ThreadManager.h"

#include "Camera.h"
#include "DrawFunction.h"

#include <cassert>
#include <cmath>
#include <limits>

#ifdef max
#undef max
#endif

namespace {
    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    float Clamp01(float x) {
        if (x < 0.0f) return 0.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }

    // 端に近いほど endRadius、中央ほど centerRadius
    float CalcRadiusBySegmentIndex(size_t segIndex,
        size_t segCount,
        float centerRadius,
        float endRadius,
        int relaxSegments) {
        if (segCount == 0) {
            return centerRadius;
        }

        size_t distFromStart = segIndex;
        size_t distFromEnd = (segCount - 1) - segIndex;
        size_t distFromEdge = (distFromStart < distFromEnd) ? distFromStart : distFromEnd;

        if (relaxSegments <= 0) {
            return centerRadius;
        }

        float t = 1.0f - Clamp01(
            static_cast<float>(distFromEdge) / static_cast<float>(relaxSegments));
        return Lerp(centerRadius, endRadius, t);
    }

    Vector3 NormalizeXZ(const Vector3& v) {
        float len = std::sqrt(v.x * v.x + v.z * v.z);
        if (len <= 0.0001f) {
            return {0.0f, 0.0f, 0.0f};
        }
        return {v.x / len, 0.0f, v.z / len};
    }

    float DistanceSqXZ(const Vector3& a, const Vector3& b) {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        return dx * dx + dz * dz;
    }
}

/// <summary>
/// 初期化
/// </summary>
void ThreadManager::Initialize(int maxThreads, int nodesPerThread,
    Camera* camera) {
    assert(camera);
    camera_ = camera;

    maxThreads_ = maxThreads;
    nodesPerThread_ = nodesPerThread;

    physicsList_.reserve(maxThreads_);

    renderer_ = std::make_unique<ThreadRenderer>();
    renderer_->Initialize(maxThreads, nodesPerThread, 0.02f, 6);
}

/// <summary>
/// 更新
/// </summary>
void ThreadManager::Update() {
    std::vector<std::vector<PhysicsNode>> allNodes;
    allNodes.reserve(physicsList_.size());

    for (auto& physics : physicsList_) {
        physics->Update();
        allNodes.push_back(physics->GetNodes());
    }

    renderer_->Update(allNodes, camera_);
}

/// <summary>
/// 描画
/// </summary>
void ThreadManager::Draw() {
    renderer_->Draw();
}

void ThreadManager::AddThread(const Vector3& startPos, const Vector3& endPos) {
    if (physicsList_.size() >= maxThreads_) {
        return;
    }

    auto physics = std::make_unique<ThreadPhysics>();
    physics->Initialize(startPos, endPos, nodesPerThread_);

    physicsList_.push_back(std::move(physics));
}

void ThreadManager::ClearThreads() {
    physicsList_.clear();
}

bool ThreadManager::IsOnThread(const Vector3& pos, float radius) const {
    float radiusSq = radius * radius;

    Vector3 flatPos = {pos.x, 0.0f, pos.z};

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        for (size_t i = 0; i < nodes.size() - 1; ++i) {
            Vector3 flatA = {nodes[i].currentPos.x, 0.0f, nodes[i].currentPos.z};
            Vector3 flatB = {nodes[i + 1].currentPos.x, 0.0f, nodes[i + 1].currentPos.z};

            float distSq = DistanceSqPointToSegment(flatPos, flatA, flatB);

            if (distSq <= radiusSq) {
                return true;
            }
        }
    }
    return false;
}

void ThreadManager::ApplyPlayerWeight(const Vector3& pos, float radius, float weight) {
    Vector3 force = {0.0f, -weight, 0.0f};

    for (auto& physics : physicsList_) {
        physics->ApplyForce(pos, radius, force);
    }
}

bool ThreadManager::GetThreadHeight(const Vector3& pos, float radius, float& outY) const {
    float radiusSq = radius * radius;
    Vector3 flatPos = {pos.x, 0.0f, pos.z};

    float closestDistSq = radiusSq + 1.0f;
    bool found = false;
    float bestY = 0.0f;

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) continue;

        for (size_t i = 0; i < nodes.size() - 1; ++i) {
            Vector3 flatA = {nodes[i].currentPos.x, 0.0f, nodes[i].currentPos.z};
            Vector3 flatB = {nodes[i + 1].currentPos.x, 0.0f, nodes[i + 1].currentPos.z};

            Vector3 ab = flatB - flatA;
            Vector3 ap = flatPos - flatA;

            float ab2 = ab.x * ab.x + ab.z * ab.z;
            float t = 0.0f;
            if (ab2 > 0.0f) {
                t = (ap.x * ab.x + ap.z * ab.z) / ab2;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
            }

            Vector3 closestFlat = {flatA.x + ab.x * t, 0.0f, flatA.z + ab.z * t};
            float dx = flatPos.x - closestFlat.x;
            float dz = flatPos.z - closestFlat.z;
            float distSq = dx * dx + dz * dz;

            if (distSq <= radiusSq) {
                if (distSq < closestDistSq) {
                    closestDistSq = distSq;
                    found = true;

                    float yA = nodes[i].currentPos.y;
                    float yB = nodes[i + 1].currentPos.y;
                    bestY = yA + (yB - yA) * t;
                }
            }
        }
    }

    if (found) {
        outY = bestY;
        return true;
    }
    return false;
}

bool ThreadManager::IsOnThreadWithEndRelax(const Vector3& pos,
    float centerRadius,
    float endRadius,
    int relaxSegments) const {
    ThreadPointInfo info;
    return GetClosestThreadPointWithEndRelax(
        pos, centerRadius, endRadius, relaxSegments, info);
}

bool ThreadManager::GetThreadHeightWithEndRelax(const Vector3& pos,
    float centerRadius,
    float endRadius,
    int relaxSegments,
    float& outY) const {
    ThreadPointInfo info;
    if (!GetClosestThreadPointWithEndRelax(
        pos, centerRadius, endRadius, relaxSegments, info)) {
        return false;
    }

    outY = info.closestPos.y;
    return true;
}

bool ThreadManager::GetClosestThreadPointWithEndRelax(const Vector3& pos,
    float centerRadius,
    float endRadius,
    int relaxSegments,
    ThreadPointInfo& outInfo) const {

    outInfo = ThreadPointInfo {};
    Vector3 flatPos = {pos.x, 0.0f, pos.z};

    bool found = false;

    for (size_t threadIndex = 0; threadIndex < physicsList_.size(); ++threadIndex) {
        const auto& nodes = physicsList_[threadIndex]->GetNodes();
        if (nodes.size() < 2) {
            continue;
        }

        const size_t segCount = nodes.size() - 1;

        for (size_t i = 0; i < segCount; ++i) {
            Vector3 flatA = {nodes[i].currentPos.x, 0.0f, nodes[i].currentPos.z};
            Vector3 flatB = {nodes[i + 1].currentPos.x, 0.0f, nodes[i + 1].currentPos.z};

            Vector3 ab = flatB - flatA;
            Vector3 ap = flatPos - flatA;

            float ab2 = ab.x * ab.x + ab.z * ab.z;
            float t = 0.0f;
            if (ab2 > 0.0f) {
                t = (ap.x * ab.x + ap.z * ab.z) / ab2;
                t = Clamp01(t);
            }

            Vector3 closestFlat = {
                flatA.x + ab.x * t,
                0.0f,
                flatA.z + ab.z * t
            };

            float distSq = DistanceSqXZ(flatPos, closestFlat);

            float localRadius = CalcRadiusBySegmentIndex(
                i, segCount, centerRadius, endRadius, relaxSegments);

            if (distSq > localRadius * localRadius) {
                continue;
            }

            if (!found || distSq < outInfo.distSq) {
                found = true;

                float yA = nodes[i].currentPos.y;
                float yB = nodes[i + 1].currentPos.y;
                float y = yA + (yB - yA) * t;

                outInfo.hit = true;
                outInfo.threadIndex = threadIndex;
                outInfo.segmentIndex = i;
                outInfo.segmentT = t;
                outInfo.closestPos = {closestFlat.x, y, closestFlat.z};
                outInfo.tangent = NormalizeXZ(flatB - flatA);
                outInfo.distSq = distSq;

                Vector3 flatStart = {nodes.front().currentPos.x, 0.0f, nodes.front().currentPos.z};
                Vector3 flatEnd = {nodes.back().currentPos.x, 0.0f, nodes.back().currentPos.z};

                outInfo.distToStartSq = DistanceSqXZ(outInfo.closestPos, flatStart);
                outInfo.distToEndSq = DistanceSqXZ(outInfo.closestPos, flatEnd);
            }
        }
    }

    return found;
}

bool ThreadManager::GetClosestPointOnThread(size_t threadIndex,
    const Vector3& pos,
    ThreadPointInfo& outInfo) const {

    outInfo = ThreadPointInfo {};

    if (threadIndex >= physicsList_.size()) {
        return false;
    }

    const auto& nodes = physicsList_[threadIndex]->GetNodes();
    if (nodes.size() < 2) {
        return false;
    }

    Vector3 flatPos = {pos.x, 0.0f, pos.z};
    bool found = false;

    for (size_t i = 0; i < nodes.size() - 1; ++i) {
        Vector3 flatA = {nodes[i].currentPos.x, 0.0f, nodes[i].currentPos.z};
        Vector3 flatB = {nodes[i + 1].currentPos.x, 0.0f, nodes[i + 1].currentPos.z};

        Vector3 ab = flatB - flatA;
        Vector3 ap = flatPos - flatA;

        float ab2 = ab.x * ab.x + ab.z * ab.z;
        float t = 0.0f;
        if (ab2 > 0.0f) {
            t = (ap.x * ab.x + ap.z * ab.z) / ab2;
            t = Clamp01(t);
        }

        Vector3 closestFlat = {
            flatA.x + ab.x * t,
            0.0f,
            flatA.z + ab.z * t
        };

        float distSq = DistanceSqXZ(flatPos, closestFlat);

        if (!found || distSq < outInfo.distSq) {
            found = true;

            float yA = nodes[i].currentPos.y;
            float yB = nodes[i + 1].currentPos.y;
            float y = yA + (yB - yA) * t;

            outInfo.hit = true;
            outInfo.threadIndex = threadIndex;
            outInfo.segmentIndex = i;
            outInfo.segmentT = t;
            outInfo.closestPos = {closestFlat.x, y, closestFlat.z};
            outInfo.tangent = NormalizeXZ(flatB - flatA);
            outInfo.distSq = distSq;
        }
    }

    if (!found) {
        return false;
    }

    Vector3 flatStart = {nodes.front().currentPos.x, 0.0f, nodes.front().currentPos.z};
    Vector3 flatEnd = {nodes.back().currentPos.x, 0.0f, nodes.back().currentPos.z};

    outInfo.distToStartSq = DistanceSqXZ(outInfo.closestPos, flatStart);
    outInfo.distToEndSq = DistanceSqXZ(outInfo.closestPos, flatEnd);

    return true;
}