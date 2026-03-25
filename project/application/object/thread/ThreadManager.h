#pragma once

#include "ThreadPhysics.h"
#include "ThreadRenderer.h"
#include <Vector4.h>
#include <memory>
#include <vector>

class Camera;

class ThreadManager {
public:
    struct ThreadIntersection {
        Vector3 position = {};
        float radius = 3.0f;

        size_t threadIndexA = 0;
        size_t threadIndexB = 0;

        size_t segmentIndexA = 0;
        size_t segmentIndexB = 0;

        Vector3 segmentAStart = {};
        Vector3 segmentAEnd = {};

        Vector3 segmentBStart = {};
        Vector3 segmentBEnd = {};
    };

    struct ThreadQueryResult {
        bool hit = false;
        Vector3 closestPoint = {};
        Vector3 startPoint = {};
        Vector3 endPoint = {};

        Vector3 segmentStart = {};
        Vector3 segmentEnd = {};

        float t = 0.0f;
    };

public:
    void Initialize(int maxThreads, int nodesPerThread, Camera* camera);
    void Update();
    void Draw();

    void AddThread(const Vector3& startPos, const Vector3& endPos);
    void ClearThreads();

    bool IsOnThread(const Vector3& pos, float radius) const;
    void ApplyPlayerWeight(const Vector3& pos, float radius, float weight);
    bool GetThreadHeight(const Vector3& pos, float radius, float& outY) const;

    bool FindNearestThread(const Vector3& pos, float radius, ThreadQueryResult& outResult) const;

    const std::vector<ThreadIntersection>& GetIntersections() const { return intersections_; }
    const std::vector<std::unique_ptr<ThreadPhysics>>& GetPhysicsList() const { return physicsList_; }

private:
    std::vector<std::unique_ptr<ThreadPhysics>> physicsList_;
    std::unique_ptr<ThreadRenderer> renderer_;
    Camera* camera_ = nullptr;
    int maxThreads_ = 0;
    int nodesPerThread_ = 0;

    std::vector<ThreadIntersection> intersections_;

private:
    void CalculateIntersections();
};