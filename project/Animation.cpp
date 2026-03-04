#include "Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <imgui.h>
Animation::AnimationData Animation::LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
    AnimationData animation;
    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene* scene = importer.ReadFile(filePath.c_str(),0);
    assert(scene->mNumAnimations!=0);
    aiAnimation* animationAssimp = scene->mAnimations[0];
float ticksPerSecond = (float)(animationAssimp->mTicksPerSecond != 0 ? animationAssimp->mTicksPerSecond : 25.0f);
    animation.duration = static_cast<float>(animationAssimp->mDuration / ticksPerSecond);

    for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex)    {
        aiNodeAnim* nodeAnimationAssimp=animationAssimp->mChannels[channelIndex];
        NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
        for (uint32_t keyIndex = 0; keyIndex <nodeAnimationAssimp->mNumPositionKeys ; ++keyIndex)
        {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
            KeyFrameVector3 keyframe;
            keyframe.time = static_cast<float>(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = { keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z };
            nodeAnimation.translate.keyFrames.push_back(keyframe);
        }
        for (uint32_t keyIndex = 0; keyIndex <nodeAnimationAssimp->mNumPositionKeys ; ++keyIndex)
        {
            aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
            KeyFrameQuaternion keyframe;
            keyframe.time = static_cast<float>(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = { keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z ,keyAssimp.mValue.w};
            nodeAnimation.rotate.keyFrames.push_back(keyframe);
        }
        // 3. スケール (Scaling) - 必要であれば追加
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
            KeyFrameVector3 keyframe;
            keyframe.time = static_cast<float>(keyAssimp.mTime / ticksPerSecond);
            keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
            nodeAnimation.scale.keyFrames.push_back(keyframe);
        }
    }


    return animation;


}
void Animation::Initialize(const std::string& directoryPath, const std::string& filename)
{
    AnimeData_ = LoadAnimationFile(directoryPath,filename);
}

void Animation::Update()
{
    currentTime_ += 0.016f; // 仮の時間の進行（例: 60 FPSで約16msごとに更新）
    if (currentTime_ > AnimeData_.duration) {
        currentTime_ = 0.0f; // アニメーションをループさせる
    }
}