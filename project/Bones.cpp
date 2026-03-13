#include "Bones.h"
#include "Camera.h"
#include "Object3dCommon.h"

void Bones::Update(Skeleton& skeleton)
{
    for (Joint& joint : skeleton.joints)
    {
        joint.localMatrix = MakeAfineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
        if (joint.parent)
        { joint.skeletonSpaceMatrix=joint.localMatrix*skeleton.joints[*joint.parent].skeletonSpaceMatrix;
        }
        else
        {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}
Bones::Skeleton Bones::CreateSkelton(const Model::Node& rootNode) {
    Skeleton skeleton;
    skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

    for (const Joint& joint : skeleton.joints)
    {
        skeleton.jointMap.emplace(joint.name, joint.index);
    }
    return skeleton;
}
int32_t Bones::CreateJoint(const  Model::Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
    Joint joint;
    joint.name = node.name;
    joint.localMatrix = node.localMatrix;
    joint.skeletonSpaceMatrix = Makeidentity4x4();
    joint.transform = node.transform;
    joint.index = static_cast<int32_t>(joints.size());
    joint.parent = parent;
    joints.push_back(joint);
    for (const  Model::Node& Child : node.children) {

        int32_t childIndex = CreateJoint(Child, joint.index, joints);
        joints[joint.index].children.push_back(childIndex);

    }
    return joint.index;

}



void Bones::DrawDebugSkeleton(const Skeleton& skeleton, Camera* camera)
{
    if (!camera) {
        return;
    }

    // スケルトンの全ジョイントに対してループ
    for (const Joint& joint : skeleton.joints)
    {
        // ジョイントのワールド位置を取得
        Vector3 jointWorldPos = {
            joint.skeletonSpaceMatrix.m[3][0],
            joint.skeletonSpaceMatrix.m[3][1],
            joint.skeletonSpaceMatrix.m[3][2]
        };

        // 親がいる場合、親ジョイントとの間に線を引く
        if (joint.parent)
        {
            const Joint& parentJoint = skeleton.joints[*joint.parent];
            Vector3 parentWorldPos = {
                parentJoint.skeletonSpaceMatrix.m[3][0],
                parentJoint.skeletonSpaceMatrix.m[3][1],
                parentJoint.skeletonSpaceMatrix.m[3][2]
            };

            // ボーン（親から子への線）を描画
            // 後で線描画機能を実装する際に使用
        }
    }
}