#include "OneWayObject.h"
#include "ModelManager.h"
#include <numbers>

void OneWayObject::Initialize(const Vector3& pos, Direction dir, float width, float depth) {
    position_ = pos;
    allowedDir_ = dir;
    width_ = width;
    depth_ = depth;

    object_ = std::make_unique<Object3d>();
    object_->Initialize();


    ModelManager::GetInstance()->LoadModel("resources", "plane.obj");
    object_->SetModel("plane.obj");
    object_->SetTranslate(pos);
    object_->SetScale({ width_, 1.0f, depth_ });

    // 許可方向（allowedDir_）に合わせてモデルを回転させる
    float pi = std::numbers::pi_v<float>;
    switch (allowedDir_) {
    case Direction::PositiveX: object_->SetRotate({ 0.0f, pi, 0.0f }); break;  // 90度
    case Direction::NegativeX: object_->SetRotate({ 0.0f, 0.0f, 0.0f }); break; // -90度
    case Direction::PositiveZ: object_->SetRotate({ 0.0f, pi / 2.0f, 0.0f }); break;       // 0度（正面）
    case Direction::NegativeZ: object_->SetRotate({ 0.0f, -pi / 2.0f, 0.0f }); break;          // 180度
    }
}

void OneWayObject::Update() {
    if (object_) {
        object_->Update();
    }
}

void OneWayObject::Draw() {
    if (object_) {
        object_->Draw();
    }
}

bool OneWayObject::CanPass(const Vector3& moveDir, const Vector3& currentPos) const {
    Vector3 allowedVec = { 0, 0, 0 };
    switch (allowedDir_) {
    case Direction::PositiveX: allowedVec = { 1, 0, 0 }; break;
    case Direction::NegativeX: allowedVec = { -1, 0, 0 }; break;
    case Direction::PositiveZ: allowedVec = { 0, 0, 1 }; break;
    case Direction::NegativeZ: allowedVec = { 0, 0, -1 }; break;
    }

    // 内積で判定
    float dot = moveDir.x * allowedVec.x + moveDir.z * allowedVec.z;
    return dot > 0.0f;
}

bool OneWayObject::IsInside(const Vector3& pos) const
{
    float sizeX, sizeZ;

    // ログの結果（Zに長くなっている）を見て、あえて逆に割り当てます
    if (allowedDir_ == Direction::PositiveX || allowedDir_ == Direction::NegativeX) {
        // X方向に伸ばしたい場合
        sizeX = depth_; // 長辺(15.0)をXに
        sizeZ = width_; // 短辺(5.0)をZに
    }
    else {
        // Z方向に伸ばしたい場合
        sizeX = width_;
        sizeZ = depth_;
    }

    float minX = position_.x - (sizeX / 2.0f);
    float maxX = position_.x + (sizeX / 2.0f);
    float minZ = position_.z - (sizeZ / 2.0f);
    float maxZ = position_.z + (sizeZ / 2.0f);

    // 接続用のマージン（ここも重要）
    return (pos.x >= minX && pos.x <= maxX &&
        pos.z >= minZ  && pos.z <= maxZ);
}

OneWayObject::AABB OneWayObject::GetAABB() const {
    float realWidth = width_;
    float realDepth = depth_;
    if (allowedDir_ == Direction::PositiveX || allowedDir_ == Direction::NegativeX) {
        realWidth = depth_;
        realDepth = width_;
    }
    return {
        { position_.x - realWidth / 2.0f, 0, position_.z - realDepth / 2.0f },
        { position_.x + realWidth / 2.0f, 0, position_.z + realDepth / 2.0f }
    };
}