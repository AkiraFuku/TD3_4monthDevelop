#include "ThreadManager.h"

#include "Camera.h"
#include "DrawFunction.h"

#include <cassert>
#include <algorithm>
#include <cmath>

namespace {

    Vector3 FlattenXZ(const Vector3& v) {
        return {v.x, 0.0f, v.z};
    }

    float Clamp01(float value) {
        return std::clamp(value, 0.0f, 1.0f);
    }

} // namespace

void ThreadManager::Initialize(int maxThreads, int nodesPerThread, Camera* camera) {
    assert(camera);
    camera_ = camera;

    maxThreads_ = maxThreads;
    nodesPerThread_ = nodesPerThread;

    physicsList_.reserve(maxThreads_);

    renderer_ = std::make_unique<ThreadRenderer>();
    renderer_->Initialize(maxThreads, nodesPerThread, 0.02f, 6);
}

void ThreadManager::Update() {
    std::vector<std::vector<PhysicsNode>> allNodes;
    allNodes.reserve(physicsList_.size());

    for (auto& physics : physicsList_) {
        physics->Update();
        allNodes.push_back(physics->GetNodes());
    }

    renderer_->Update(allNodes, camera_);
}

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

bool ThreadManager::FindNearestThread(const Vector3& pos, float radius, ThreadQueryResult& outResult) const {
    if (radius <= 0.0f) {
        return false;
    }

    const float radiusSq = radius * radius;
    const Vector3 flatPos = FlattenXZ(pos);

    bool found = false;
    float bestDistSq = radiusSq + 1.0f;

    for (const auto& physics : physicsList_) {
        const auto& nodes = physics->GetNodes();
        if (nodes.size() < 2) {
            continue;
        }

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

                const float y = a.y + (b.y - a.y) * localT;

                outResult.hit = true;
                outResult.closestPoint = {closestFlat.x, y, closestFlat.z};
                outResult.startPoint = nodes.front().currentPos;
                outResult.endPoint = nodes.back().currentPos;

                // 追加
                outResult.segmentStart = a;
                outResult.segmentEnd = b;

                outResult.t = (static_cast<float>(i) + localT) / static_cast<float>(nodes.size() - 1);
            }
        }
    }

    return found;
}

bool ThreadManager::IsOnThread(const Vector3& pos, float radius) const {
    ThreadQueryResult result {};
    return FindNearestThread(pos, radius, result);
}

bool ThreadManager::GetThreadHeight(const Vector3& pos, float radius, float& outY) const {
    ThreadQueryResult result {};
    if (!FindNearestThread(pos, radius, result)) {
        return false;
    }

    outY = result.closestPoint.y;
    return true;
}

void ThreadManager::ApplyPlayerWeight(const Vector3& pos, float radius, float weight) {
    Vector3 force = {0.0f, -weight, 0.0f};

    for (auto& physics : physicsList_) {
        physics->ApplyForce(pos, radius, force);
    }
}