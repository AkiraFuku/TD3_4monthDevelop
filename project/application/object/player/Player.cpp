#include "Player.h"
#include "Input.h"
#include "ModelManager.h"
#include "imgui.h"
#include "CollisionMask.h"

#include"ThreadManager.h"
#include "Egg.h"

#include <cmath>
#include <numbers>

#include "PSOManager.h"
#include "Logger.h"

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">初期位置</param>
/// <param name="threadManager">ThreadManagerのポインタ</param>
void Player::Initialize(const Vector3& pos, ThreadManager* thread) {

    //ThreadManagerを借りる
    thread_ = thread;

    // モデルの初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    translate_ = pos;
    object_->SetTranslate(translate_);
    ModelManager::GetInstance()->LoadModel("resources", "player/player.obj");
    object_->SetModel("player/player.obj");

    PsoConfig config {};
    config.vsPath = L"resources/shaders/PLayer/Player.vs.hlsl";
    config.psPath = L"resources/shaders/PLayer/PLayer.ps.hlsl";


    config.rootSignatureGenerator = []() {
        std::vector<D3D12_ROOT_PARAMETER> rootParameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
        D3D12_STATIC_SAMPLER_DESC sampler {};
        sampler = PSOManager::GetInstance()->StaticSamplers();

        staticSamplers.push_back(sampler);
        D3D12_DESCRIPTOR_RANGE descRangeTexture[1] {};
        descRangeTexture[0].BaseShaderRegister = 0; // t0
        descRangeTexture[0].NumDescriptors = 1;
        descRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



        rootParameters.resize(8);



        // Enum定義 (可読性のため)
        enum {
            kMaterial, kTransform, kTexture, DirLight, PointLight, SpotLight, Count, kCamera
        };

        // 0. Material (CBV b0, Pixel)
        rootParameters[kMaterial].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[kMaterial].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[kMaterial].Descriptor.ShaderRegister = 0;

        // 1. Transform (CBV b0, Vertex)
        rootParameters[kTransform].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[kTransform].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[kTransform].Descriptor.ShaderRegister = 0;

        // 2. Texture (Table t0, Pixel)
        rootParameters[kTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[kTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[kTexture].DescriptorTable.pDescriptorRanges = descRangeTexture;
        rootParameters[kTexture].DescriptorTable.NumDescriptorRanges = 1;

        // ★変更: 3. DirectionalLight (SRV t1)
        rootParameters[DirLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[DirLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[DirLight].Descriptor.ShaderRegister = 1; // t1

        // ★追加: 4. PointLight (SRV t2)
        rootParameters[PointLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[PointLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[PointLight].Descriptor.ShaderRegister = 2; // t2

        // ★追加: 5. SpotLight (SRV t3)
        rootParameters[SpotLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[SpotLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[SpotLight].Descriptor.ShaderRegister = 3; // t3

        // ★追加: 6. LightCounts (CBV b3)
        rootParameters[Count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[Count].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[Count].Descriptor.ShaderRegister = 3; // b3 (b0,b1,b2は使用済みと仮定、あるいは空いている番号)
        //カメラ
        rootParameters[kCamera].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファビュー
        rootParameters[kCamera].Descriptor.ShaderRegister = 2; // レジスタ番号 2 (b2)
        rootParameters[kCamera].Descriptor.RegisterSpace = 0;
        rootParameters[kCamera].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーのみ見える

        // シリアライズ
        D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
        descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        descriptionRootSignature.pParameters = rootParameters.data();
        descriptionRootSignature.NumParameters = (UINT) rootParameters.size();
        descriptionRootSignature.pStaticSamplers = staticSamplers.data();
        descriptionRootSignature.NumStaticSamplers = (UINT) staticSamplers.size();


        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
        hr = DXCommon::GetInstance()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        assert(SUCCEEDED(hr));



        return rootSignature;
        };
    config.inputLayoutGenerator = []() {
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        };
    // 深度設定
    config.depthEnable = true;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    // PSOManagerに名前を付けて登録
    PSOManager::GetInstance()->RegisterPsoGenerator("PLayer", config);
    object_->SetPsoName("PLayer");

    // 待機状態で初期化
    ChangeState(std::make_unique<PlayerStateIdle>());
}
/// <summary>
/// 終了
/// </summary>
void Player::Finalize() {}
/// <summary>
/// 更新
/// </summary>
void Player::Update() {

    moveVel_ = {0.0f, 0.0f, 0.0f};

    if (state_) {
        state_->Update(this);
    }

    if (onThread_) {
        ResolveThreadMove();
    } else {
        IsCollisionSDF();
    }

    ResultMove();
    object_->Update();
}
/// <summary>
/// 描画
/// </summary>
void Player::Draw() {
    // モデルの描画
    object_->Draw();
}

/// <summary>
/// 移動処理
/// </summary>
void Player::UpdateMove(Vector3& moveDirection) {
    moveDirection = {0.0f, 0.0f, 0.0f};

    if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.x += 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.x += 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.x -= 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.x -= 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D)) {
        moveDirection.x += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A)) {
        moveDirection.x -= 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.z += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.z -= 1.0f;
    } else {
        return;
    }

    float length = std::sqrtf(moveDirection.x * moveDirection.x +
        moveDirection.z * moveDirection.z);

    if (length > 0.0f) {
        moveDirection.x /= length;
        moveDirection.z /= length;
    }

    // 先にThread移動を試す
    if (TryMoveOnThread(moveDirection)) {
        return;
    }

    // 通常移動
    TurnToDirection(moveDirection);

    moveVel_.x += moveDirection.x * velocity_.x;
    moveVel_.z += moveDirection.z * velocity_.z;
}

void Player::ResultMove() {
    translate_ += moveVel_;
    object_->SetTranslate(translate_);
}

void Player::IsCollisionSDF() {
    Vector3 nextPos = {};
    nextPos.x = translate_.x + moveVel_.x;
    nextPos.z = translate_.z + moveVel_.z;

    // 2. 矩形の四隅のオフセット（中心からの距離）
    float hW = kWidth * 0.5f;
    float hH = kHeight * 0.5f;

    // 四隅の座標リスト
    Vector2 corners[4] = {
        {nextPos.x - hW, nextPos.z - hH}, // 左下
        {nextPos.x + hW, nextPos.z - hH}, // 右下
        {nextPos.x - hW, nextPos.z + hH}, // 左上
        {nextPos.x + hW, nextPos.z + hH}  // 右上
    };

    // 3. 四隅の中で「最も壁に近い点」を探す
    float minDist = 10000.0f;
    Vector2 targetCorner = {nextPos.x, nextPos.z};


    for (const auto& corner : corners) {
        float d = CollisionMask::GetInstance()->GetSDFValue(corner.x, corner.y);
        if (d < minDist) {
            minDist = d;
            targetCorner = corner;
        }
    }

    // 4. 衝突判定（最も近い点が「中」に入っていたら）
    // 矩形判定の場合、理想的な距離（閾値）は 0 です。
    if (minDist < 0.075f) {
        // 最もめり込んでいる点の法線を取得
        Vector2 normal = CollisionMask::GetInstance()->GetSDFNormal(targetCorner.x, targetCorner.y);

        if (std::abs(normal.x) > 0.0001f || std::abs(normal.y) > 0.0001f)
        {
            // 押し戻し量
            float pushBack = 0.075f - minDist;

            // 座標を補正
            moveVel_.x += normal.x * pushBack;
            moveVel_.z += normal.y * pushBack;

            // 速度の射影（滑り処理）
            float dot = moveVel_.x * normal.x + moveVel_.z * normal.y;
            if (dot < 0) {
                moveVel_.x -= dot * normal.x;
                moveVel_.z -= dot * normal.y;
            }

        }
    }
}

/// <summary>
/// 状態遷移
/// </summary>
/// <param name="newState">次の状態</param>
void Player::ChangeState(std::unique_ptr<IPlayerState> newState) {
    state_ = std::move(newState);
    state_->Initialize(this);
}

/// <summary>
/// 糸を発射する処理
void Player::FireThread() {
    if (!thread_) {
        return;
    }

    if (!CanFireThread()) {
        return;
    }

    Vector3 forward = GetForward();
    Vector3 playerPos = GetPosition();

    rayResult_ = CollisionMask::GetInstance()->CastRayThroughWall(playerPos, forward, 50.0f);

    if (!rayResult_.hit) {
        return;
    }

    Vector3 start = {rayResult_.hitPos.x, 0.0f, rayResult_.hitPos.y};
    Vector3 end = {rayResult_.exitPos.x, 0.0f, rayResult_.exitPos.y};

    Vector3 dir = end - start;
    float len = std::sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (len > 0.0001f) {
        dir.x /= len;
        dir.z /= len;

        const float extend = 0.4f;
        start.x -= dir.x * extend;
        start.z -= dir.z * extend;
        end.x += dir.x * extend;
        end.z += dir.z * extend;
    }

    thread_->AddThread(start, end);
}

void Player::SetPosition(const Vector3& pos) {
    translate_ = pos;
    object_->SetTranslate(translate_);
}

// 向いている方向
Vector3 Player::GetForward() const {
    return {std::sin(rotationY_), 0.0f, std::cos(rotationY_)};
}

AABB Player::GetAABB() const {
    Vector3 worldPos = GetPosition();
    AABB aabb;

    aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
    aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

    return aabb;
}

Matrix4x4 Player::GetWorldMatrix() const {
    Matrix4x4 worldMatrix = MakeAfineMatrix(scale_, rotate_, translate_);
    return worldMatrix;
}

bool Player::CanFireThread() const {
    if (!egg_) {
        return true;
    }

    // Eggに触れている間も、Eggを持っている間も発射禁止
    if (egg_->IsHit() || egg_->IsOnPlayer()) {
        return false;
    }

    return true;
}

void Player::TurnToDirection(const Vector3& direction) {
    if (std::abs(direction.x) < 0.0001f && std::abs(direction.z) < 0.0001f) {
        return;
    }

    float targetAngleY = std::atan2(direction.x, direction.z);
    float diffrence = targetAngleY - rotationY_;

    while (diffrence > std::numbers::pi_v<float>) {
        diffrence -= 2.0f * std::numbers::pi_v<float>;
    }
    while (diffrence < -std::numbers::pi_v<float>) {
        diffrence += 2.0f * std::numbers::pi_v<float>;
    }

    rotationY_ += diffrence * kTurnSpeed;

    rotate_ = {0.0f, rotationY_, 0.0f};
    object_->SetRotate(rotate_);
}

bool Player::TryMoveOnThread(const Vector3& moveDirection) {
    if (!thread_) {
        return false;
    }

    ThreadManager::ThreadQueryResult query {};

    Vector3 probePos = translate_;
    const float probeDistance = kWidth * 0.5f + kThreadEnterRadius;
    probePos.x += moveDirection.x * probeDistance;
    probePos.z += moveDirection.z * probeDistance;

    bool found = false;

    if (onThread_) {
        // 乗っている最中は現在位置優先で見る
        found = thread_->FindNearestThread(translate_, kThreadStickRadius, query);
        if (!found) {
            found = thread_->FindNearestThread(probePos, kThreadStickRadius, query);
        }
    } else {
        // 降りた直後の再吸着を防ぐため、off中は probePos のみで判定する
        found = thread_->FindNearestThread(probePos, kThreadEnterRadius, query);
    }

    if (!found) {
        onThread_ = false;
        return false;
    }

    // 今いる線分の接線方向
    Vector3 tangent = query.segmentEnd - query.segmentStart;
    tangent.y = 0.0f;

    float tangentLength = std::sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);
    if (tangentLength <= 0.0001f) {
        tangent = query.endPoint - query.startPoint;
        tangent.y = 0.0f;
        tangentLength = std::sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);

        if (tangentLength <= 0.0001f) {
            moveVel_ = {0.0f, 0.0f, 0.0f};
            return true;
        }
    }

    tangent.x /= tangentLength;
    tangent.z /= tangentLength;

    // 入力をThread方向へ射影
    float along = moveDirection.x * tangent.x + moveDirection.z * tangent.z;

    // 端から外向きに行くなら降りる
    if (onThread_) {
        bool atStart = query.t <= kThreadExitThreshold;
        bool atEnd = query.t >= (1.0f - kThreadExitThreshold);

        bool leavingFromStart = atStart && (along < -0.0001f);
        bool leavingFromEnd = atEnd && (along > 0.0001f);

        if (leavingFromStart || leavingFromEnd) {
            onThread_ = false;
            return false; // 通常移動へ
        }
    }

    onThread_ = true;
    threadStart_ = query.startPoint;
    threadEnd_ = query.endPoint;

    moveVel_.x = tangent.x * along * velocity_.x;
    moveVel_.z = tangent.z * along * velocity_.z;

    if (std::abs(along) > 0.0001f) {
        Vector3 faceDir = tangent;
        if (along < 0.0f) {
            faceDir.x *= -1.0f;
            faceDir.z *= -1.0f;
        }
        TurnToDirection(faceDir);
    }

    return true;
}

void Player::ResolveThreadMove() {
    if (!thread_) {
        onThread_ = false;
        IsCollisionSDF();
        return;
    }

    ThreadManager::ThreadQueryResult query{};

    // 今フレームの予定移動先
    Vector3 nextPos = translate_;
    nextPos.x += moveVel_.x;
    nextPos.z += moveVel_.z;

    bool found = thread_->FindNearestThread(nextPos, kThreadStickRadius, query);
    if (!found) {
        found = thread_->FindNearestThread(translate_, kThreadStickRadius, query);
    }

    if (!found) {
        onThread_ = false;
        IsCollisionSDF();
        return;
    }

    onThread_ = true;
    threadStart_ = query.startPoint;
    threadEnd_ = query.endPoint;

    // 今いる線分の接線方向
    Vector3 tangent = query.segmentEnd - query.segmentStart;
    tangent.y = 0.0f;

    float tangentLength = std::sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);
    if (tangentLength <= 0.0001f) {
        tangent = query.endPoint - query.startPoint;
        tangent.y = 0.0f;
        tangentLength = std::sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);

        if (tangentLength <= 0.0001f) {
            return;
        }
    }

    tangent.x /= tangentLength;
    tangent.z /= tangentLength;

    // 最近点との差
    Vector3 toClosest = {
        query.closestPoint.x - nextPos.x,
        0.0f,
        query.closestPoint.z - nextPos.z
    };

    // 最近点との差を「接線方向成分」と「横方向成分」に分解
    float alongError = toClosest.x * tangent.x + toClosest.z * tangent.z;

    Vector3 lateral = {
        toClosest.x - tangent.x * alongError,
        0.0f,
        toClosest.z - tangent.z * alongError
    };

    // 端に近いほど横補正を弱める
    float edgeFade = 1.0f;

    if (query.t < kThreadEndSnapFadeRange) {
        edgeFade = query.t / kThreadEndSnapFadeRange;
    } else if (query.t > (1.0f - kThreadEndSnapFadeRange)) {
        edgeFade = (1.0f - query.t) / kThreadEndSnapFadeRange;
    }

    edgeFade = std::clamp(edgeFade, 0.0f, 1.0f);

    // 横方向だけ補正する
    moveVel_.x += lateral.x * kThreadLateralFollowStrength * edgeFade;
    moveVel_.z += lateral.z * kThreadLateralFollowStrength * edgeFade;

    // Yだけは糸に合わせる
    translate_.y = query.closestPoint.y;

    thread_->ApplyPlayerWeight(query.closestPoint, kThreadWeightRadius, kThreadWeight);
}