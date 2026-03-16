#include "ThreadPhysics.h"
#include "MathFunction.h" // Length() などの数学関数用

#include <cmath>
#include <algorithm>

namespace {
    // ゼロ除算回避などのための微小値
    constexpr float EPSILON = 1e-6f;
    constexpr float PIN_LENGTH_EPSILON = 0.0001f;
    constexpr float TENSION_FACTOR = 0.95f; // ピン留め時の張りの強さ
}

void ThreadPhysics::Initialize(const Vector3& startPos, const Vector3& endPos, int nodeCount) {
    if (nodeCount < 2) return;

    nodes_.clear();
    nodes_.reserve(nodeCount);

    Vector3 difference = endPos - startPos;
    float totalLength = Length(difference);

    float initialSegmentLength = totalLength / static_cast<float>(nodeCount - 1);
    segmentLengths_.assign(nodeCount - 1, initialSegmentLength);

    for (int i = 0; i < nodeCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(nodeCount - 1);
        Vector3 pos = startPos + (difference * t);

        PhysicsNode node;
        node.currentPos = pos;
        node.previousPos = pos;
        // 両端の質量を0（固定）、それ以外を1（可動）とする
        node.mass = (i == 0 || i == nodeCount - 1) ? 0.0f : 1.0f;

        nodes_.push_back(node);
    }
}

void ThreadPhysics::Update() {
    if (isPinAnimating_) {
        pinProgress_ = std::min(pinProgress_ + pinAnimSpeed_, 1.0f);

        float easeT = EaseOutCubic(pinProgress_);

        for (size_t i = 0; i < nodes_.size(); ++i) {
            nodes_[i].currentPos = startPositions_[i] + (targetPositions_[i] - startPositions_[i]) * easeT;
            nodes_[i].previousPos = nodes_[i].currentPos;
            nodes_[i].mass = 0.0f; // アニメーション中は全ノードを物理演算対象外(固定)とする
        }

        // アニメーション終了処理
        if (pinProgress_ >= 1.0f) {
            isPinAnimating_ = false;
            segmentLengths_ = targetSegmentLengths_; // 自然長を更新

            // 終了後は両端とピン留め位置以外を再び可動(mass=1.0)に戻す
            for (size_t i = 0; i < nodes_.size(); ++i) {
                nodes_[i].mass = (i == 0 || i == nodes_.size() - 1 || i == pinIndex_) ? 0.0f : 1.0f;
            }
        }
    }
    else {
        Integrate();
        SolveConstraints();
    }
}

void ThreadPhysics::Integrate() {
    for (auto& node : nodes_) {
        if (node.mass <= 0.0f) continue; // 固定ノード

        // Verlet積分: 現在の速度 = 現在の位置 - 過去の位置
        Vector3 velocity = node.currentPos - node.previousPos;

        node.previousPos = node.currentPos;
        node.currentPos += (velocity * damping_) + gravity_;
    }
}

void ThreadPhysics::SolveConstraints() {
    if (nodes_.size() < 2) return;

    for (int iter = 0; iter < iterations_; ++iter) {
        for (size_t i = 0; i < nodes_.size() - 1; ++i) {
            PhysicsNode& n1 = nodes_[i];
            PhysicsNode& n2 = nodes_[i + 1];

            // 両方固定なら計算スキップ
            if (n1.mass <= 0.0f && n2.mass <= 0.0f) continue;

            Vector3 diff = n1.currentPos - n2.currentPos;
            float currentDist = Length(diff);

            if (currentDist <= EPSILON) continue;

            float targetLen = segmentLengths_[i];
            // 距離の差分比率
            float differenceRatio = (targetLen - currentDist) / currentDist;

            // 質量0を固定として扱い、ウェイト(逆質量)を計算
            float invMass1 = (n1.mass > 0.0f) ? 1.0f : 0.0f;
            float invMass2 = (n2.mass > 0.0f) ? 1.0f : 0.0f;
            float weightSum = invMass1 + invMass2;

            Vector3 correction = diff * (differenceRatio / weightSum);

            // 位置の補正
            if (invMass1 > 0.0f) n1.currentPos += correction * invMass1;
            if (invMass2 > 0.0f) n2.currentPos -= correction * invMass2;
        }
    }
}

void ThreadPhysics::ApplyForce(const Vector3& pos, float radius, const Vector3& force) {
    if (radius <= 0.0f) return;

    float radiusSq = radius * radius;

    for (auto& node : nodes_) {
        if (node.mass <= 0.0f) continue;

        Vector3 diff = node.currentPos - pos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq <= radiusSq) {
            float dist = std::sqrt(distSq);
            float falloff = 1.0f - (dist / radius);
            node.currentPos += force * falloff;
        }
    }
}

void ThreadPhysics::PinNodeNearPosition(const Vector3& targetPos) {
    if (nodes_.size() < 3) return;

    Vector3 startPos = nodes_.front().currentPos;
    Vector3 endPos = nodes_.back().currentPos;

    float len1 = Length(targetPos - startPos);
    float len2 = Length(endPos - targetPos);
    float totalLen = len1 + len2;

    if (totalLen <= PIN_LENGTH_EPSILON) return;

    // 比率からピン留めするノードのインデックスを決定 (両端は選ばない)
    int pinIndex = static_cast<int>(std::round((len1 / totalLen) * (nodes_.size() - 1)));
    pinIndex = std::clamp(pinIndex, 1, static_cast<int>(nodes_.size()) - 2);

    SetupPinAnimationTarget(startPos, endPos, targetPos, pinIndex, len1, len2);
}

void ThreadPhysics::SetupPinAnimationTarget(const Vector3& startPos, const Vector3& endPos, const Vector3& targetPos, int pinIndex, float len1, float len2) {
    isPinAnimating_ = true;
    pinProgress_ = 0.0f;
    pinIndex_ = pinIndex;

    size_t nodeCount = nodes_.size();
    startPositions_.resize(nodeCount);
    targetPositions_.resize(nodeCount);
    targetSegmentLengths_.resize(nodeCount - 1);

    // 1. 各セグメントの目標自然長の計算
    float segLen1 = (len1 / static_cast<float>(pinIndex)) * TENSION_FACTOR;
    for (int i = 0; i < pinIndex; ++i) {
        targetSegmentLengths_[i] = segLen1;
    }

    float segLen2 = (len2 / static_cast<float>(nodeCount - 1 - pinIndex)) * TENSION_FACTOR;
    for (size_t i = pinIndex; i < nodeCount - 1; ++i) {
        targetSegmentLengths_[i] = segLen2;
    }

    // 2. 現在の位置をアニメーション開始位置として保存
    for (size_t i = 0; i < nodeCount; ++i) {
        startPositions_[i] = nodes_[i].currentPos;
    }

    // 3. 目標位置の計算（左側：startPos ～ targetPos）
    for (int i = 0; i <= pinIndex; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(pinIndex);
        targetPositions_[i] = startPos + ((targetPos - startPos) * t);
    }

    // 4. 目標位置の計算（右側：targetPos ～ endPos）
    for (size_t i = pinIndex + 1; i < nodeCount; ++i) {
        float t = static_cast<float>(i - pinIndex) / static_cast<float>(nodeCount - 1 - pinIndex);
        targetPositions_[i] = targetPos + ((endPos - targetPos) * t);
    }
}

float ThreadPhysics::EaseOutCubic(float t) {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}