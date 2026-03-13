#pragma once

#include "Transform.h"
#include "MathFunction.h"
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <map>
#include "Model.h"
#include <optional>

class Camera;

class Bones
{
public:

    struct Joint {
        QuaternionTransform transform{}; // 明示的にデフォルト初期化
        Matrix4x4 localMatrix = Makeidentity4x4(); // 明示的に初期化
        Matrix4x4 skeletonSpaceMatrix = Makeidentity4x4(); // ジョイントのスケルトンスペース行列
        std::string name;
        std::vector<int32_t> children; // 子ジョイントのインデックス
        int32_t index = 0; // ジョイントのインデックスを明示的に初期化
        std::optional<int32_t> parent; // 親ジョイントのインデックス（ルートジョイントの場合はnullopt）
    };

    struct Skeleton {
        int32_t root; // ルートジョイントのインデックス
        std::map<std::string, int32_t> jointMap; // ジョイント名からインデックスへのマップ
        std::vector<Joint> joints; // ジョイントの配列

    };


    void Update(Skeleton& skeleton);

    
    void DrawDebugSkeleton(const Skeleton& skeleton, Camera* camera);

    Skeleton CreateSkelton(const Model::Node& rootNode);
    int32_t CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent,std::vector<Joint>& joint);

};

