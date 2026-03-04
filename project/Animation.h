#pragma once
#include "Vector3.h"
#include "Quanternion.h"
#include <map>
#include <vector>
#include <string>
template <typename tValue>
struct KeyFrame
{
    tValue value;
    float time;
};
using KeyFrameVector3 = KeyFrame<Vector3>;
using KeyFrameQuaternion = KeyFrame<Quaternion>;


template <typename tValue>
struct AnimationCurve
{
    std::vector<KeyFrame<tValue>> keyFrames;
};

struct  NodeAnimation {
    AnimationCurve<Vector3> translate;
    AnimationCurve<Quaternion> rotate;
    AnimationCurve<Vector3> scale;
};

struct Animation
{
    float duration; // アニメーションの全体の長さ（秒）
    std::map<std::string, NodeAnimation> nodeAnimations; // ノード名とそのアニメーションデータのマップ
};

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);


