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

    bool SegmentIntersectXZ(
        const Vector3& p0, const Vector3& p1,
        const Vector3& p2, const Vector3& p3,
        Vector3& outIntersect) {
        float s1_x = p1.x - p0.x;
        float s1_z = p1.z - p0.z;
        float s2_x = p3.x - p2.x;
        float s2_z = p3.z - p2.z;

        float det = (-s2_x * s1_z + s1_x * s2_z);
        if (std::abs(det) < 1e-6f) {
            return false;
        }

        float s = (-s1_z * (p0.x - p2.x) + s1_x * (p0.z - p2.z)) / det;
        float t = (s2_x * (p0.z - p2.z) - s2_z * (p0.x - p2.x)) / det;

        if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
            outIntersect.x = p0.x + (t * s1_x);
            outIntersect.y = p0.y + (t * (p1.y - p0.y));
            outIntersect.z = p0.z + (t * s1_z);
            return true;
        }

        return false;
    }

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

    CalculateIntersections();
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
    intersections_.clear();
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
                outResult.segmentStart = a;
                outResult.segmentEnd = b;
                outResult.t = (static_cast<float>(i) + localT) / static_cast<float>(nodes.size() - 1);
            }
        }
    }

    return found;
}

void ThreadManager::CalculateIntersections() {
    intersections_.clear();

    if (physicsList_.size() < 2) {
        return;
    }

    for (size_t i = 0; i < physicsList_.size(); ++i) {
        const auto& nodesA = physicsList_[i]->GetNodes();
        if (nodesA.size() < 2) {
            continue;
        }

        for (size_t j = i + 1; j < physicsList_.size(); ++j) {
            const auto& nodesB = physicsList_[j]->GetNodes();
            if (nodesB.size() < 2) {
                continue;
            }

            for (size_t a = 0; a + 1 < nodesA.size(); ++a) {
                for (size_t b = 0; b + 1 < nodesB.size(); ++b) {
                    Vector3 intersectPos {};
                    if (!SegmentIntersectXZ(
                        nodesA[a].currentPos, nodesA[a + 1].currentPos,
                        nodesB[b].currentPos, nodesB[b + 1].currentPos,
                        intersectPos)) {
                        continue;
                    }

                    if (IsNearSameIntersection(intersections_, intersectPos)) {
                        continue;
                    }

                    ThreadIntersection intersection {};
                    intersection.position = intersectPos;
                    intersection.radius = 0.8f;

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