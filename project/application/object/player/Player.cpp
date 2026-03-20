#include "Player.h"
#include "Input.h"
#include "ModelManager.h"
#include "imgui.h"
#include "CollisionMask.h"

#include"ThreadManager.h"

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

#ifdef USE_IMGUI

    ImGui::Begin("Player Window");

    Vector3 scale = object_->GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
        object_->SetScale(scale);
    }

    Vector3 rotate = object_->GetRotate();
    if (ImGui::DragFloat3("Rotate", &rotate.x, 0.1f, -360.0f, 360.0f)) {
        object_->SetRotate(rotate);
    }

    Vector3 translate = translate_;
    if (ImGui::DragFloat3("Translate", &translate.x, 0.1f, -100.0f, 100.0f)) {
        translate_ = translate;
    }

    Vector3 moveVel = moveVel_;
    if (ImGui::DragFloat3("Move Velocity", &moveVel.x, 0.01f, -1.0f, 1.0f)) {
        moveVel_ = moveVel;
    }

    ImGui::End();

    CollisionMask::RayResult res =
        CollisionMask::GetInstance()->CastRayThroughWall(translate_, GetForward(), 50.0f);
    if (res.hit) {
        ImGui::Begin("Ray Debug");
        ImGui::Text("Hit World: %.2f, %.2f", res.hitPos.x, res.hitPos.y);

        auto mask = CollisionMask::GetInstance()->GetMaskData(
            CollisionMask::GetInstance()->GetCurrentMaskMap());
        float hitU = (res.hitPos.x - mask->min_.x) / (mask->max_.x - mask->min_.x);
        float hitV = (res.hitPos.y - mask->min_.y) / (mask->max_.y - mask->min_.y);
        ImGui::Text("Hit Pixel: %.1f, %.1f",
            hitU * mask->textureData.widthX,
            hitV * mask->textureData.widthZ);
        ImGui::End();
    }

#endif

    // すでに糸に乗ってロック中の時だけ、糸方向移動を適用
    if (threadLocked_) {
        ConstrainMoveToThread();
    }

    IsCollisionSDF();
    ResultMove();

    // 移動後に糸判定
    UpdateThreadInteraction();

    object_->SetTranslate(translate_);
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
    // 移動方向を蓄積する変数
    moveDirection = {0.0f, 0.0f, 0.0f};

    if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_W))
    {
        moveDirection.x += 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D) && Input::GetInstance()->PushedKeyDown(DIK_S))
    {
        moveDirection.x += 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_S))
    {
        moveDirection.x -= 0.7f;
        moveDirection.z -= 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A) && Input::GetInstance()->PushedKeyDown(DIK_W))
    {
        moveDirection.x -= 0.7f;
        moveDirection.z += 0.7f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_D))
    {
        moveDirection.x += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A))
    {
        moveDirection.x -= 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_W))
    {
        moveDirection.z += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_S))
    {
        moveDirection.z -= 1.0f;
    } else
    {
        moveDirection.x = 0.0f;
        moveDirection.z = 0.0f;
    }



    if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
        // ベクトルの長さを計算
        float length = std::sqrtf(moveDirection.x * moveDirection.x +
            moveDirection.z * moveDirection.z);
        // 正規化
        if (length != 0.0f) {
            moveDirection.x /= length;
            moveDirection.z /= length;
        }

        // 入力方向から角度を計算
        float targetAngleY = std::atan2(moveDirection.x, moveDirection.z);

        // 目標角度と現在の角度の差分
        float diffrence = targetAngleY - rotationY_;

        // 差分を-π ~ πの範囲に収める(最短経路で旋回)
        while (diffrence > std::numbers::pi_v<float>) {
            diffrence -= 2.0f * std::numbers::pi_v<float>;
        }
        while (diffrence < -std::numbers::pi_v<float>) {
            diffrence += 2.0f * std::numbers::pi_v<float>;
        }

        // 現在の角度に差分の一部を加算
        rotationY_ += diffrence * kTurnSpeed;

        // モデルに適用
        rotate_ = {0.0f, rotationY_, 0.0f};
        object_->SetRotate(rotate_);

        // 正規化した方向に指定速度を掛けて加算
        moveVel_.x += moveDirection.x * velocity_.x;
        moveVel_.z += moveDirection.z * velocity_.z;
    }


}

void Player::IsCollision() {
    float nextX = translate_.x + moveVel_.x;
    float nextZ = translate_.z + moveVel_.z;

    // --- X軸方向の判定 ---
    if (CollisionMask::GetInstance()->IsCollisionWall(nextX, translate_.z, kWidth)) {
        Vector3 nextPos = {nextX, translate_.y, translate_.z};
        if (!thread_ || !thread_->IsOnThread(nextPos, threadWalkRadius_)) {
            moveVel_.x = 0.0f;
        }
    }

    // --- Z軸方向の判定 ---
    if (CollisionMask::GetInstance()->IsCollisionWall(translate_.x, nextZ, kWidth)) {
        Vector3 nextPos = {translate_.x, translate_.y, nextZ};
        if (!thread_ || !thread_->IsOnThread(nextPos, threadWalkRadius_)) {
            moveVel_.z = 0.0f;
        }
    }
}

void Player::ResultMove() {
    translate_ += moveVel_;
    object_->SetTranslate(translate_);
}

void Player::IsCollisionSDF() {
    Vector3 nextPos = translate_ + moveVel_;

    if (IsWalkablePosition(nextPos)) {
        return;
    }

    float dist = CollisionMask::GetInstance()->GetSDFValue(nextPos.x, nextPos.z);
    float playerRadius = kWidth * 0.5f;

    if (dist < playerRadius) {
        Vector2 normal = CollisionMask::GetInstance()->GetSDFNormal(nextPos.x, nextPos.z);

        if (std::abs(normal.x) < 0.001f && std::abs(normal.y) < 0.001f) {
            moveVel_.x = 0.0f;
            moveVel_.z = 0.0f;
            return;
        }

        float penetration = playerRadius - dist;
        moveVel_.x += normal.x * penetration;
        moveVel_.z += normal.y * penetration;
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
/// </summary>
void Player::FireThread() {
    if (!thread_) {
        return;
    }

    // 向いている方向
    Vector3 forward = GetForward();
    // プレイヤーの位置
    Vector3 playerPos = GetPosition();

    rayResult_ = CollisionMask::GetInstance()->
        CastRayThroughWall(playerPos, forward, 50.0f);

    //// 始点（プレイヤーの中心より少し上から出すなど、調整可能）
    //Vector3 startPos = {playerPos.x, playerPos.y, playerPos.z};

    //// 終点（向いている方向に距離を掛けた位置）
    //float threadLength = 10.0f; // 糸を飛ばす距離を指定
    //Vector3 endPos = {
    //    startPos.x + forward.x * threadLength,
    //    startPos.y + forward.y * threadLength,
    //    startPos.z + forward.z * threadLength
    //};

    //// ThreadManagerに糸の生成を依頼
    //thread_->AddThread(startPos, endPos);

    // ThreadManagerに糸の生成を依頼
    thread_->AddThread(
        {rayResult_.hitPos.x, 0.0f, rayResult_.hitPos.y},
        {rayResult_.exitPos.x, 0.0f, rayResult_.exitPos.y});
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

/// <summary>
/// 糸の相互作用
/// </summary>
void Player::UpdateThreadInteraction() {
    if (!thread_) {
        UnlockThread();
        translate_.y = 0.0f;
        return;
    }

    thread_->ApplyPlayerWeight(translate_, threadInfluenceRadius_, threadPlayerWeight_);

    // 降りた直後は、少し糸から離れるまで再乗車禁止
    if (threadRelockBlocked_) {
        if (thread_->IsOnThread(translate_, threadStayRadius_)) {
            onThread_ = false;
            translate_.y = 0.0f;
            return;
        }
        threadRelockBlocked_ = false;
    }

    float threadY = 0.0f;

    // まだ乗っていない時
    if (!onThread_) {
        if (thread_->GetThreadHeight(translate_, threadEnterRadius_, threadY)) {
            onThread_ = true;
            translate_.y = threadY;

            ThreadManager::ThreadPointInfo enterInfo;
            if (thread_->GetClosestThreadPointWithEndRelax(
                translate_,
                threadCenterRadius_,
                threadEndRadius_,
                threadEndSegments_,
                enterInfo)) {
                threadLocked_ = true;
                lockedThreadIndex_ = enterInfo.threadIndex;
            }
        } else {
            onThread_ = false;
            translate_.y = 0.0f;
        }
        return;
    }

    // すでに乗っている時は少し広めで維持
    if (thread_->GetThreadHeight(translate_, threadStayRadius_, threadY)) {
        translate_.y = threadY;
    } else {
        UnlockThread();
        translate_.y = 0.0f;
    }
}

Matrix4x4 Player::GetWorldMatrix() const {
    Matrix4x4 worldMatrix = MakeAfineMatrix(scale_, rotate_, translate_);
    return worldMatrix;
}

bool Player::IsWalkablePosition(const Vector3& pos) const {
    float dist = CollisionMask::GetInstance()->GetSDFValue(pos.x, pos.z);
    float playerRadius = kWidth * 0.5f;

    // 普通の地面として歩ける
    if (dist >= playerRadius) {
        return true;
    }

    // まだ乗っていなくても、糸の上は進入可能にする
    if (thread_ && thread_->IsOnThreadWithEndRelax(
        pos,
        threadCenterRadius_,
        threadEndRadius_,
        threadEndSegments_)) {
        return true;
    }

    return false;
}

void Player::UnlockThread() {
    threadLocked_ = false;
    lockedThreadIndex_ = kInvalidThreadIndex;
    onThread_ = false;
    threadRelockBlocked_ = true;
}

void Player::ConstrainMoveToThread() {
    if (!thread_) {
        return;
    }

    if (!threadLocked_) {
        return;
    }

    ThreadManager::ThreadPointInfo currentInfo;
    if (!thread_->GetClosestPointOnThread(lockedThreadIndex_, translate_, currentInfo)) {
        UnlockThread();
        return;
    }

    Vector3 rawMove = moveVel_;
    float rawLenSq = rawMove.x * rawMove.x + rawMove.z * rawMove.z;

    if (rawLenSq <= 0.000001f) {
        translate_.y = currentInfo.closestPos.y;
        onThread_ = true;
        return;
    }

    const float leaveRadiusSq = threadLeaveEndRadius_ * threadLeaveEndRadius_;
    bool atStart = currentInfo.distToStartSq <= leaveRadiusSq;
    bool atEnd = currentInfo.distToEndSq <= leaveRadiusSq;
    bool canLeave = atStart || atEnd;

    float tangentDot =
        rawMove.x * currentInfo.tangent.x +
        rawMove.z * currentInfo.tangent.z;

    bool wantsExit = false;
    if (atStart) {
        wantsExit = (tangentDot <= 0.0f);
    } else if (atEnd) {
        wantsExit = (tangentDot >= 0.0f);
    }

    // ここが変更点
    float rawLen = std::sqrt(rawLenSq);
    Vector3 rawDir = {
        rawMove.x / rawLen,
        0.0f,
        rawMove.z / rawLen
    };

    // 1フレーム先ではなく、プレイヤー半径ぶん先を見る
    float exitCheckDistance = kWidth * 0.5f + 0.1f;
    Vector3 exitCheckPos = {
        translate_.x + rawDir.x * exitCheckDistance,
        translate_.y,
        translate_.z + rawDir.z * exitCheckDistance
    };

    if (canLeave && wantsExit && IsGroundWalkablePosition(exitCheckPos)) {
        UnlockThread();
        moveVel_ = rawMove;
        return;
    }

    Vector3 projectedMove = {
        currentInfo.tangent.x * tangentDot,
        0.0f,
        currentInfo.tangent.z * tangentDot
    };

    Vector3 projectedNextPos = translate_ + projectedMove;

    ThreadManager::ThreadPointInfo nextInfo;
    if (!thread_->GetClosestPointOnThread(lockedThreadIndex_, projectedNextPos, nextInfo)) {
        UnlockThread();
        return;
    }

    moveVel_.x = nextInfo.closestPos.x - translate_.x;
    moveVel_.z = nextInfo.closestPos.z - translate_.z;
    translate_.y = nextInfo.closestPos.y;
    onThread_ = true;

    if (std::abs(tangentDot) > 0.0001f) {
        Vector3 faceDir = nextInfo.tangent;
        if (tangentDot < 0.0f) {
            faceDir.x *= -1.0f;
            faceDir.z *= -1.0f;
        }

        rotationY_ = std::atan2(faceDir.x, faceDir.z);
        rotate_ = {0.0f, rotationY_, 0.0f};
        object_->SetRotate(rotate_);
    }
}

bool Player::IsGroundWalkablePosition(const Vector3& pos) const {
    float dist = CollisionMask::GetInstance()->GetSDFValue(pos.x, pos.z);
    float playerRadius = kWidth * 0.5f;
    return dist >= playerRadius;
}