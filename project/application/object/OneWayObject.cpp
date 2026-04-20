#include "OneWayObject.h"
#include "ModelManager.h"
#include <numbers>

#include "imgui.h" // 追加
#include <string>

void OneWayObject::Initialize(const Vector3& pos, Direction dir, float width, float depth) {
    position_ = pos;
    allowedDir_ = dir;
    width_ = width;
    depth_ = depth;

    object_ = std::make_unique<Object3d>();
    object_->Initialize();


    ModelManager::GetInstance()->LoadModel("resources", "oneWay/oneWay.obj");
    object_->SetModel("oneWay/oneWay.obj");
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

    object_->SetScale({sizeX, 1.0f, sizeZ});

    float minX = position_.x - (sizeX / 2.0f);
    float maxX = position_.x + (sizeX / 2.0f);
    float minZ = position_.z - (sizeZ / 2.0f);
    float maxZ = position_.z + (sizeZ / 2.0f);

    // 接続用のマージン（ここも重要）
    return (pos.x >= minX && pos.x <= maxX &&
        pos.z >= minZ  && pos.z <= maxZ);
}

void OneWayObject::ResolveCollision(Vector3& currentPos, Vector3& moveVel) const {
    AABB aabb = GetAABB();

    // プレイヤーの当たり判定の半径（Player::kWidth * 0.5f 程度）
    // Player.h で kWidth = 0.8f なので 0.4f。余裕を見て 0.45f〜0.5f に設定します。
    float radius = 0.45f;

    // 進入を許可する側（入り口付近）にいる場合は、通り抜けを許すため何もしない
    if (IsAtEntrySide(currentPos)) {
        return;
    }

    // AABBをプレイヤーの半径分だけ膨らませた「衝突判定領域」を計算
    float minX = aabb.min.x - radius;
    float maxX = aabb.max.x + radius;
    float minZ = aabb.min.z - radius;
    float maxZ = aabb.max.z + radius;

    // 次のフレームの予定地
    Vector3 nextPos = currentPos + moveVel;

    // 次のフレームで判定領域の中に入ってしまうかチェック
    if (nextPos.x > minX && nextPos.x < maxX && nextPos.z > minZ && nextPos.z < maxZ) {

        // 現在の座標（衝突前）が各境界のどこにいたかを判定
        bool wasOutsideLeft = currentPos.x <= minX;
        bool wasOutsideRight = currentPos.x >= maxX;
        bool wasOutsideFront = currentPos.z <= minZ;
        bool wasOutsideBack = currentPos.z >= maxZ;

        // 押し戻す際の微小な余白
        const float kEpsilon = 0.001f;

        // --- 押し戻し処理 ---
        // 各面について「そこが入り口（allowedDir）ではない場合」に壁として機能させる

        if (wasOutsideLeft) {
            // 左側 (-X) からの衝突。+X方向行きの一方通行でなければ押し戻す
            if (allowedDir_ != Direction::PositiveX) {
                currentPos.x = minX - kEpsilon;
                moveVel.x = 0.0f;
            }
        } else if (wasOutsideRight) {
            // 右側 (+X) からの衝突。-X方向行きの一方通行でなければ押し戻す
            if (allowedDir_ != Direction::NegativeX) {
                currentPos.x = maxX + kEpsilon;
                moveVel.x = 0.0f;
            }
        } else if (wasOutsideFront) {
            // 手前 (-Z) からの衝突。+Z方向行きの一方通行でなければ押し戻す
            if (allowedDir_ != Direction::PositiveZ) {
                currentPos.z = minZ - kEpsilon;
                moveVel.z = 0.0f;
            }
        } else if (wasOutsideBack) {
            // 奥 (+Z) からの衝突。-Z方向行きの一方通行でなければ押し戻す
            if (allowedDir_ != Direction::NegativeZ) {
                currentPos.z = maxZ + kEpsilon;
                moveVel.z = 0.0f;
            }
        }
    }
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

void OneWayObject::DebugDrawImGui(int index, const Vector3& playerPos, const Vector3& playerMoveVel) const {
    std::string treeName = "OneWayObject[" + std::to_string(index) + "]";

    // ツリーを展開して詳細を表示
    if (ImGui::TreeNode(treeName.c_str())) {
        AABB aabb = GetAABB();
        ImGui::Text("AABB Min : (%.2f, %.2f, %.2f)", aabb.min.x, aabb.min.y, aabb.min.z);
        ImGui::Text("AABB Max : (%.2f, %.2f, %.2f)", aabb.max.x, aabb.max.y, aabb.max.z);

        ImGui::Separator();

        // プレイヤー情報
        ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", playerPos.x, playerPos.y, playerPos.z);
        ImGui::Text("Player Vel: (%.2f, %.2f, %.2f)", playerMoveVel.x, playerMoveVel.y, playerMoveVel.z);

        Vector3 nextPos = {playerPos.x + playerMoveVel.x, playerPos.y + playerMoveVel.y, playerPos.z + playerMoveVel.z};

        // 当たり判定フラグの確認
        bool isInside = IsInside(nextPos);
        ImGui::Text("IsInside(Next) : %s", isInside ? "TRUE" : "FALSE");

        bool canPass = CanPass(playerMoveVel, playerPos);
        ImGui::Text("CanPass(Vel)   : %s", canPass ? "TRUE" : "FALSE");

        // 許可されている進行方向
        const char* dirStr = "";
        switch (allowedDir_) {
        case Direction::PositiveX: dirStr = "+X"; break;
        case Direction::NegativeX: dirStr = "-X"; break;
        case Direction::PositiveZ: dirStr = "+Z"; break;
        case Direction::NegativeZ: dirStr = "-Z"; break;
        }
        ImGui::Text("Allowed Dir    : %s", dirStr);

        ImGui::TreePop();
    }
}

bool OneWayObject::IsAtEntrySide(const Vector3& playerPos) const {
    AABB aabb = GetAABB();
    // 進入を許可する「厚み」の定義（お好みで調整してください）
    const float kEntryThreshold = 1.0f;

    switch (allowedDir_) {
    case Direction::PositiveX:
        // +X方向への一方通行なら、入り口は X の最小値側
        return playerPos.x <= aabb.min.x + kEntryThreshold;

    case Direction::NegativeX:
        // -X方向への一方通行なら、入り口は X の最大値側
        return playerPos.x >= aabb.max.x - kEntryThreshold;

    case Direction::PositiveZ:
        // +Z方向への一方通行なら、入り口は Z の最小値側
        return playerPos.z <= aabb.min.z + kEntryThreshold;

    case Direction::NegativeZ:
        // -Z方向への一方通行なら、入り口は Z の最大値側
        return playerPos.z >= aabb.max.z - kEntryThreshold;
    }
    return false;
}