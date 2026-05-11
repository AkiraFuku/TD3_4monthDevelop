#include "Player.h"
#include "Input.h"
#include "ModelManager.h"
#include "imgui.h"
#include "CollisionMask.h"
#include "BrokenBlock.h"

#include"ThreadManager.h"
#include "Egg.h"
#include "OneWayObject.h"

#include <cmath>
#include <numbers>

#include "PSOManager.h"
#include "Logger.h"

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">初期位置</param>
/// <param name="threadManager">ThreadManagerのポインタ</param>
void Player::Initialize(const Vector3& pos, ThreadManager* thread)
{

    //ThreadManagerを借りる
    thread_ = thread;

    // モデルの初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    translate_ = pos;
    object_->SetTranslate(translate_);
    InitializeModel();

    // 予測線の初期化
    predictionLineObj_ = std::make_unique<Object3d>();
    predictionLineObj_->Initialize();
    predictionLineObj_->SetModel("cylinder/cylinder.obj");

    predictionPointObj_ = std::make_unique<Object3d>();
    predictionPointObj_->Initialize();
    // もし circle.obj などがあればそれを使用。なければ sphere.obj で代用
    predictionPointObj_->SetModel("sphere/sphere.obj");

    // PSOを生成
    CreatePSO();

    // 待機状態で初期化
    ChangeState(std::make_unique<PlayerStateIdle>());

    anima_ = std::make_unique<PlayerAnima>();
    anima_->Initialize(object_.get());
    anima_->Play();

    anima_->ChangeAnimation(PlayerAnima::AnimationState::Idle);

    playerGroup_.items["position"] = JSONManager::Item {translate_};
    playerGroup_.items["rotate"] = JSONManager::Item {rotate_};
    playerGroup_.items["velocity"] = JSONManager::Item {velocity_};
    playerGroup_.items["remainingThreadCount"] = JSONManager::Item {remainingThreadCount_};

    JSONManager::GetInstance()->RegisterGroup("Player", playerGroup_);

    // サウンド読み込み
    threadSound_ = Audio::GetInstance()->LoadAudio("resources/sounds/thread.wav");

}
/// <summary>
/// 終了
/// </summary>
void Player::Finalize()
{
}
/// <summary>
/// 更新
/// </summary>
void Player::Update()
{
    moveVel_ = {0.0f, 0.0f, 0.0f};

    UpdatePredictionLine();

    if (state_) {
        state_->Update(this);
    }

    // ★これを追加！！
    if (onThread_) {
        // 【Threadに乗っている場合】
        // 糸の上では自由落下しないため落下速度をリセット
        fallSpeed_ = 0.0f;

        // 糸に沿った移動補正、Y座標の追従、糸への重さ適用
        ResolveThreadMove();
    }

#ifdef USE_IMGUI

    ImGui::Begin("Player");

    ImGui::Text("B / RB: FireThread\n\nSPACE / A: EggOnPlayer");

    // 座標の表示と編集 (DragFloat3 はドラッグで数値を変更できます)
    if (ImGui::DragFloat3("Position", &translate_.x, 0.1f)) {
        // ImGuiで値を書き換えた場合、即座にオブジェクトに反映する
        object_->SetTranslate(translate_);
    }

    // 状態の表示
    ImGui::Text("On Thread: %s", onThread_ ? "Yes" : "No");
    ImGui::Text("Can Draw Prediction: %s", canDrawPrediction_ ? "Yes" : "No");
    ImGui::Text("Remaining Threads: %d", remainingThreadCount_);

    // 速度（移動量）の確認
    ImGui::Text("Move Velocity: (%.2f, %.2f, %.2f)", moveVel_.x, moveVel_.y, moveVel_.z);

    // 回転角の確認
    float degrees = rotationY_ * 180.0f / std::numbers::pi_v<float>;
    ImGui::Text("Rotation Y: %.2f deg", degrees);

    ImGui::End();

#endif

    IsCollisionOneWay();

    ResultMove();

#ifdef _DEBUG

    ImGui::Begin("Player Json");

    if (ImGui::Button("Save"))
    {
        SaveJson();
    }

    if (ImGui::Button("Load"))
    {
        LoadJson();
    }

    ImGui::InputFloat3("Position", &translate_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

    ImGui::End();

#endif

#ifdef USE_IMGUI
    // 既存の ImGui 処理の近くに追加
    ImGui::Begin("OneWayObject Debug");
    for (size_t i = 0; i < oneWayObjects_.size(); ++i) {
        if (oneWayObjects_[i]) {
            // 現在のプレイヤー位置と移動速度を渡す
            oneWayObjects_[i]->DebugDrawImGui(static_cast<int>(i), translate_, moveVel_);
        }
    }
    ImGui::End();
#endif

    anima_->Update();
    object_->Update();
}
/// <summary>
/// 描画
/// </summary>
void Player::Draw()
{
    // モデルの描画
    object_->Draw();

    if (canDrawPrediction_) {
        // 線の描画
        if (predictionLineObj_) {
            predictionLineObj_->Draw();
        }
        // ★追加：ポインターの描画
        if (predictionPointObj_) {
            predictionPointObj_->Draw();
        }
    }
}

/// <summary>
/// 移動処理
/// </summary>
void Player::Move(const Vector3& moveDirection)
{
    //  moveDirectionは既にState側で計算・正規化されている前提

    // 先にThread移動を試す
    if (TryMoveOnThread(moveDirection)) {
        return;
    }

    // 通常移動
    TurnToDirection(moveDirection);

    moveVel_.x += moveDirection.x * velocity_.x;
    moveVel_.z += moveDirection.z * velocity_.z;
}

void Player::ResultMove()
{
    translate_ += moveVel_;
    object_->SetTranslate(translate_);
}

void Player::IsCollisionSDF()
{
    Vector3 nextPos = {};
    nextPos.x = translate_.x + moveVel_.x;
    nextPos.z = translate_.z + moveVel_.z;

    // 2. 矩形の四隅のオフセット（中心からの距離）
    float hW = kWidth * 0.5f;
    float hH = kHeight * 0.5f;

    // =========================================================
    // ★ 追加: BrokenBlock から意図せず壁の中に落ちないようにする制限処理
    // =========================================================
    BrokenBlock* activeBlock = nullptr;
    // 現在、プレイヤーの中心がどのブロックに乗っているかを取得
    for (auto* block : brokenBlocks_) {
        if (block && !block->IsBroken() && block->IsInside(translate_)) {
            activeBlock = block;
            break;
        }
    }

    if (activeBlock) {
        AABB bAABB = activeBlock->GetAABB();

        // プレイヤーの中心座標がブロックの範囲内に収まるための限界値（キャラクターの幅を考慮）
        float limitMinX = bAABB.min.x + hW;
        float limitMaxX = bAABB.max.x - hW;
        float limitMinZ = bAABB.min.z + hH;
        float limitMaxZ = bAABB.max.z - hH;

        // --- X軸の制限 ---
        if (nextPos.x < limitMinX) {
            // ブロックから出ようとしている先が「壁」なら、ブロックの端に留める
            if (CollisionMask::GetInstance()->GetSDFValue(limitMinX - hW, nextPos.z) < 0.075f) {
                moveVel_.x = limitMinX - translate_.x;
            }
        } else if (nextPos.x > limitMaxX) {
            if (CollisionMask::GetInstance()->GetSDFValue(limitMaxX + hW, nextPos.z) < 0.075f) {
                moveVel_.x = limitMaxX - translate_.x;
            }
        }

        // --- Z軸の制限（X軸の補正を反映した上で判定） ---
        nextPos.x = translate_.x + moveVel_.x;
        if (nextPos.z < limitMinZ) {
            if (CollisionMask::GetInstance()->GetSDFValue(nextPos.x, limitMinZ - hH) < 0.075f) {
                moveVel_.z = limitMinZ - translate_.z;
            }
        } else if (nextPos.z > limitMaxZ) {
            if (CollisionMask::GetInstance()->GetSDFValue(nextPos.x, limitMaxZ + hH) < 0.075f) {
                moveVel_.z = limitMaxZ - translate_.z;
            }
        }

        // 制限をかけた後の速度で nextPos を更新
        nextPos.x = translate_.x + moveVel_.x;
        nextPos.z = translate_.z + moveVel_.z;
    }
    // =========================================================

    // 四隅の座標リスト
    Vector2 corners[4] = {
        {nextPos.x - hW, nextPos.z - hH}, // 左下
        {nextPos.x + hW, nextPos.z - hH}, // 右下
        {nextPos.x - hW, nextPos.z + hH}, // 左上
        {nextPos.x + hW, nextPos.z + hH}  // 右上
    };

    // =========================================================
    // ★追加: ループの前に「どのくらい OneWayObject に乗っているか」を判定する
    // =========================================================
    OneWayObject* activeOneWay = nullptr;
    for (auto* oneWay : oneWayObjects_) {
        if (!oneWay) continue;

        int insideCount = 0; // OneWayObjectの中に入っている頂点の数
        for (const auto& corner : corners) {
            Vector3 cornerPos = {corner.x, translate_.y, corner.y};
            if (oneWay->IsInside(cornerPos)) {
                insideCount++;
            }
        }

        // 四隅のうち2つ以上（半分以上の面積）入っていれば有効とみなす
        // ※「完全に乗り切ったら」にしたい場合は >= 4 に調整してください
        if (insideCount >= 4) {
            activeOneWay = oneWay;
            break;
        }
    }

    // ★修正点: 4つの頂点「すべて」に対して順番にめり込み判定と押し戻しを行う
    for (const auto& corner : corners) {
        // =========================================================
        // ★ 修正: この頂点が BrokenBlock の中にあるか判定する
        // =========================================================
        bool isCornerOnBrokenBlock = false;
        for (auto* block : brokenBlocks_) {
            if (block && !block->IsBroken()) {
                // corner は Vector2 なので Vector3 に変換して判定
                Vector3 corner3D = {corner.x, translate_.y, corner.y};
                if (block->IsInside(corner3D)) {
                    isCornerOnBrokenBlock = true;
                    break;
                }
            }
        }

        // ★ ブロックの中にある頂点は、壁(SDF)の押し戻しをスキップする！
        if (isCornerOnBrokenBlock) {
            continue;
        }

        // 以降は既存の押し戻し処理
        float d = CollisionMask::GetInstance()->GetSDFValue(corner.x, corner.y);

        // 衝突判定（この頂点が壁にめり込んでいる場合）
        if (d < 0.075f) {
            // めり込んでいる点の法線を取得
            Vector2 normal = CollisionMask::GetInstance()->GetSDFNormal(corner.x, corner.y);

            if (std::abs(normal.x) > 0.0001f || std::abs(normal.y) > 0.0001f)
            {
                // 押し戻し量
                float pushBack = 0.075f - d;
                Vector2 pushVec = {normal.x * pushBack, normal.y * pushBack};

                // =========================================================
                // ★修正: 事前に判定した activeOneWay の結果を使うように変更
                // =========================================================
                bool isOnOneWay = false;
                OneWayObject::Direction overrideDir = OneWayObject::Direction::PositiveZ; // 初期化

                if (activeOneWay) {
                    isOnOneWay = true;
                    overrideDir = activeOneWay->GetDirection();
                }

                if (isOnOneWay) {
                    // 壁からの押し戻し方向と OneWayObject の進行方向を比較し、
                    // 通行を許可する方向への壁の押し戻し成分だけをゼロにする
                    if (overrideDir == OneWayObject::Direction::PositiveZ && pushVec.y < 0.0f) {
                        pushVec.y = 0.0f; normal.y = 0.0f;
                    } else if (overrideDir == OneWayObject::Direction::NegativeZ && pushVec.y > 0.0f) {
                        pushVec.y = 0.0f; normal.y = 0.0f;
                    } else if (overrideDir == OneWayObject::Direction::PositiveX && pushVec.x < 0.0f) {
                        pushVec.x = 0.0f; normal.x = 0.0f;
                    } else if (overrideDir == OneWayObject::Direction::NegativeX && pushVec.x > 0.0f) {
                        pushVec.x = 0.0f; normal.x = 0.0f;
                    }

                    // 法線の一部をゼロにした場合、滑り処理を正しく行うため再正規化する
                    float len = std::sqrt(normal.x * normal.x + normal.y * normal.y);
                    if (len > 0.0001f) {
                        normal.x /= len;
                        normal.y /= len;
                    } else {
                        normal.x = 0.0f;
                        normal.y = 0.0f;
                    }
                }

                // 座標を補正 (計算した pushVec を足す)
                moveVel_.x += pushVec.x;
                moveVel_.z += pushVec.y;

                // 速度の射影（滑り処理）
                float dot = moveVel_.x * normal.x + moveVel_.z * normal.y;
                if (dot < 0) {
                    moveVel_.x -= dot * normal.x;
                    moveVel_.z -= dot * normal.y;
                }
            }
        }
    }
}

/// <summary>
/// 状態遷移
/// </summary>
/// <param name="newState">次の状態</param>
void Player::ChangeState(std::unique_ptr<IPlayerState> newState)
{
    state_ = std::move(newState);
    state_->Initialize(this);
}

/// <summary>
/// 糸を発射する処理
void Player::FireThread()
{
    // ★追加：残り回数が0なら張れずにリターンする
    if (remainingThreadCount_ <= 0) {
        return;
    }

    if (!thread_ || onThread_) {
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

    const float targetY = CollisionMask::GetInstance()->GetTranslate().y;

    Vector3 start = {rayResult_.hitPos.x, targetY, rayResult_.hitPos.y};
    Vector3 end = {rayResult_.exitPos.x, targetY, rayResult_.exitPos.y};

    // =========================================================
    // ★ 追加: 既存の糸と近すぎないか（重複しないか）チェック
    // =========================================================
    if (!thread_->CanCreateThread(start, end, kMinThreadCreateDistance)) {
        return; // 近すぎる場合はここで処理を抜け、予測線を描画しない
    }

    Vector3 dir = end - start;
    float len = std::sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (len > 0.0001f) {
        dir.x /= len;
        dir.z /= len;

        /*const float extend = 0.2f;
        start.x -= dir.x * extend;
        start.z -= dir.z * extend;*/
        //end.x += dir.x * extend;
        //end.z += dir.z * extend;
    }

    thread_->AddThread(start, end);
    didFireThread_ = true;

    // サウンド再生
    Audio::GetInstance()->PlayAudio(threadSound_, false, 1.0f);

    remainingThreadCount_--;
}

void Player::CreatePSO()
{
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
        enum
        {
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
}

void Player::SetPosition(const Vector3& pos)
{
    translate_ = pos;
    object_->SetTranslate(translate_);
}

// 向いている方向
Vector3 Player::GetForward() const
{
    return {std::sin(rotationY_), 0.0f, std::cos(rotationY_)};
}

AABB Player::GetAABB() const
{
    Vector3 worldPos = GetPosition();
    AABB aabb;

    aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
    aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

    return aabb;
}

Matrix4x4 Player::GetWorldMatrix() const
{
    Matrix4x4 worldMatrix = MakeAfineMatrix(scale_, rotate_, translate_);
    return worldMatrix;
}

bool Player::CanFireThread() const
{
    if (!egg_) {
        return true;
    }

    // Eggに触れている間も、Eggを持っている間も発射禁止
    if (egg_->IsOnPlayer()) {
        return false;
    }

    return true;
}

OneWayObject* Player::CheckOnOneWayObject() const
{
    Vector3 pos = GetPosition();
    for (auto* oneWay : oneWayObjects_) {
        if (oneWay && oneWay->IsInside(pos)) {
            return oneWay;
        }
    }
    return nullptr;
}

void Player::TurnToDirection(const Vector3& direction)
{
    if (std::abs(direction.x) < 0.0001f && std::abs(direction.z) < 0.0001f) {
        return;
    }

    float targetAngleY = std::atan2(direction.x, direction.z);
    float difference = targetAngleY - rotationY_;

    while (difference > std::numbers::pi_v<float>) {
        difference -= 2.0f * std::numbers::pi_v<float>;
    }
    while (difference < -std::numbers::pi_v<float>) {
        difference += 2.0f * std::numbers::pi_v<float>;
    }

    rotationY_ += difference * kTurnSpeed;

    rotate_ = {0.0f, rotationY_, 0.0f};
    object_->SetRotate(rotate_);
}

void Player::UpdatePredictionLine()
{
    // ★追加: 照準の位置を計算して更新
    const float kAimDistance = 5.0f; // プレイヤーから照準までの距離
    Vector3 forward = GetForward();

    Vector3 aimPos = {
        translate_.x + forward.x * kAimDistance,
        translate_.y + 1.0f, // 地面に埋まらないように少し浮かせる
        translate_.z + forward.z * kAimDistance
    };

    canDrawPrediction_ = false;

    // 糸を生成可能な条件が揃っているかチェック
    if (remainingThreadCount_ > 0 && thread_ && !onThread_ && CanFireThread()) {
        Vector3 playerPos = GetPosition();

        // Rayを飛ばして壁との交差を判定（FireThreadと同じ処理）
        auto rayResult = CollisionMask::GetInstance()->CastRayThroughWall(playerPos, forward, 50.0f);

        if (rayResult.hit) {
            const float targetY = CollisionMask::GetInstance()->GetTranslate().y;

            Vector3 start = {rayResult.hitPos.x, targetY, rayResult.hitPos.y};
            Vector3 end = {rayResult.exitPos.x, targetY, rayResult.exitPos.y};

            // =========================================================
            // ★ 追加: 既存の糸と近すぎないか（重複しないか）チェック
            // =========================================================
            if (!thread_->CanCreateThread(start, end, kMinThreadCreateDistance)) {
                return; // 近すぎる場合は発射をキャンセルする
            }

            Vector3 dir = end - start;
            float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

            if (distance > 0.001f) {
                canDrawPrediction_ = true;

                // --- 予測線の更新 (既存) ---
                Vector3 centerPos = {(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, (start.z + end.z) * 0.5f};
                float yaw = std::atan2(dir.x, dir.z);
                float pitch = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));

                predictionLineObj_->SetTranslate(centerPos);
                predictionLineObj_->SetScale({0.2f, 0.2f, distance}); // 太さは適宜調整
                predictionLineObj_->SetRotate({pitch, yaw, 0.0f});
                predictionLineObj_->Update();

                // ★追加：予測地点（ポインター）の更新
                // 位置はレイの終点
                predictionPointObj_->SetTranslate(end);
                // 球体を平たく潰して「円」に見せる（Y軸を小さくする）
                predictionPointObj_->SetScale({0.5f, 0.05f, 0.5f});
                predictionPointObj_->Update();
            }
        }
    }
}

void Player::IsCollisionOneWay()
{
    // 登録されている全ての OneWayObject に対して補正をかける
    for (auto* oneWay : oneWayObjects_) {
        if (oneWay) {
            oneWay->ResolveCollision(translate_, moveVel_);
        }
    }
}

void Player::InitializeModel()
{

    ModelManager::GetInstance()->LoadModel("resources", "player/player.obj");
    ModelManager::GetInstance()->LoadModel("resources", "player/Arm/playerArm.obj");
    ModelManager::GetInstance()->LoadModel("resources", "player/Leg/playerLeg.obj");

    if (object_)
    {
        object_->AddModel("player/player.obj", "Body");
        object_->AddModel("player/Arm/playerArm.obj", "Arm", "Body");
        object_->AddModel("player/Leg/playerLeg.obj", "Leg", "Body");

    }

}

void Player::SaveJson()
{
    playerGroup_.items["position"] = JSONManager::Item {translate_};
    playerGroup_.items["rotate"] = JSONManager::Item {rotate_};
    playerGroup_.items["velocity"] = JSONManager::Item {velocity_};
    playerGroup_.items["remainingThreadCount"] = JSONManager::Item {remainingThreadCount_};

    JSONManager::GetInstance()->RegisterGroup("Player", playerGroup_);
    JSONManager::GetInstance()->SaveFile("Player");
}

void Player::LoadJson()
{
    // ファイルを読み込む
    JSONManager::GetInstance()->LoadFile("Player");

    // 値を取り出す
    Vector3 pos;
    if (JSONManager::GetInstance()->TryGetVector3("Player", "position", pos)) {
        SetPosition(pos);
    }

    int32_t remaining;
    if (JSONManager::GetInstance()->TryGetInt("Player", "remainingThreadCount", remaining)) {
        remainingThreadCount_ = remaining;
    }

    ResultMove();
}

bool Player::TryMoveOnThread(const Vector3& moveDirection)
{
    if (!thread_) {
        return false;
    }

    ThreadManager::ThreadQueryResult query {};

    Vector3 probePos = translate_;
    const float probeDistance = kWidth * 0.5f + kThreadEnterRadius;
    probePos.x += moveDirection.x * probeDistance;
    probePos.z += moveDirection.z * probeDistance;

    bool found = false;

    // ★追加: 検索方向として入力方向を使う。入力が無い場合はキャラクターの向いている方向を使う
    Vector3 searchDir = moveDirection;
    if (std::abs(searchDir.x) < 0.0001f && std::abs(searchDir.z) < 0.0001f) {
        searchDir = GetForward();
    }

    if (onThread_) {
        // 乗っている最中は現在位置優先で見る
        found = thread_->FindBestThread(translate_, searchDir, kThreadStickRadius, query);
        if (!found) {
            found = thread_->FindBestThread(probePos, searchDir, kThreadStickRadius, query);
        }
    } else {
        // 降りた直後の再吸着を防ぐため、off中は probePos のみで判定する
        // ★修正: 新規吸着時は、スコア評価式の FindTargetThread を使う！
        found = thread_->FindTargetThread(probePos, searchDir, kThreadEnterRadius, query);
    }

    if (!found) {
        // ★追加: 糸から降りる時にY座標を戻す
        if (onThread_) {
            translate_.y = threadBaseY_;
        }
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

    if (onThread_ && thread_) {
        // 全ての交差点情報をチェック
        for (const auto& intersection : thread_->GetIntersections()) {
            // プレイヤーと交差点の距離（XZ平面）を計算
            float dx = translate_.x - intersection.position.x;
            float dz = translate_.z - intersection.position.z;
            float dist = std::sqrtf(dx * dx + dz * dz);

            // プレイヤーが交差点の判定半径内に入っているか
            if (dist <= intersection.radius) {
                // 交差している2本の糸の方向ベクトルを計算
                Vector3 dirA = {intersection.segmentAEnd.x - intersection.segmentAStart.x, 0.0f, intersection.segmentAEnd.z - intersection.segmentAStart.z};
                float lenA = std::sqrtf(dirA.x * dirA.x + dirA.z * dirA.z);
                if (lenA > 0.0001f) { dirA.x /= lenA; dirA.z /= lenA; }

                Vector3 dirB = {intersection.segmentBEnd.x - intersection.segmentBStart.x, 0.0f, intersection.segmentBEnd.z - intersection.segmentBStart.z};
                float lenB = std::sqrtf(dirB.x * dirB.x + dirB.z * dirB.z);
                if (lenB > 0.0001f) { dirB.x /= lenB; dirB.z /= lenB; }

                // 今の進行方向(tangent)と、2本の糸の一致度を比較する
                float diffA = std::abs(tangent.x * dirA.x + tangent.z * dirA.z);
                float diffB = std::abs(tangent.x * dirB.x + tangent.z * dirB.z);

                // 一致度が低い方 ＝ 「今乗っている糸とは違う、交差している別の糸」
                Vector3 otherTangent = (diffA < diffB) ? dirA : dirB;

                // プレイヤーのキー入力(moveDirection)が、それぞれの糸の方向とどれくらい一致しているか
                float dotCurrent = std::abs(moveDirection.x * tangent.x + moveDirection.z * tangent.z);
                float dotOther = std::abs(moveDirection.x * otherTangent.x + moveDirection.z * otherTangent.z);

                // もし「別の糸」への入力成分が十分あり、かつ「今の糸」と同じかそれ以上に入力されている場合
                // （キーボードの斜め入力で同点になった場合、曲がる意志があるとみなして乗り換えを優先する）
                if (dotOther >= dotCurrent - 0.01f && dotOther > 0.1f) {
                    tangent = otherTangent; // ★移動方向を強制的に「別の糸」へ切り替える！
                }
                break; // 交差点の処理は1つ見つかれば終了
            }
        }
    }

    // 入力をThread方向へ射影
    float along = moveDirection.x * tangent.x + moveDirection.z * tangent.z;

    // 端から外向きに行くなら降りる
    if (onThread_) {
        bool atStart = query.t <= kThreadExitThreshold;
        bool atEnd = query.t >= (1.0f - kThreadExitThreshold);

        bool leavingFromStart = atStart && (along < -0.0001f);
        bool leavingFromEnd = atEnd && (along > 0.0001f);

        if (leavingFromStart || leavingFromEnd) {
            translate_.y = threadBaseY_;
            onThread_ = false;
            return false; // 通常移動へ
        }
    }

    if (!onThread_) {
        // ★追加: PlayerのY座標と、Threadの最寄り点のY座標の差分（オフセット）を保持する
        threadOffsetY_ = translate_.y - query.closestPoint.y;
    }

    onThread_ = true;

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

void Player::ResolveThreadMove()
{
    if (!thread_) {
        onThread_ = false;
        IsCollisionSDF();
        return;
    }

    ThreadManager::ThreadQueryResult query {};

    // 今フレームの予定移動先
    Vector3 nextPos = translate_;
    nextPos.x += moveVel_.x;
    nextPos.z += moveVel_.z;

    Vector3 currentMoveDir = {moveVel_.x, 0.0f, moveVel_.z};

    // ★追加: 速度がほぼ無い場合は、正面方向を向かせる
    if (std::abs(currentMoveDir.x) < 0.0001f && std::abs(currentMoveDir.z) < 0.0001f) {
        currentMoveDir = GetForward();
    }

    bool found = false;

    // ★ 修正点: 現在の状態によって検索方法を切り替える
    if (onThread_) {
        // すでに糸に乗っている場合（交差点でのスムーズな分岐・乗り換え用）
        found = thread_->FindBestThread(nextPos, currentMoveDir, kThreadStickRadius, query);
        if (!found) {
            found = thread_->FindBestThread(translate_, currentMoveDir, kThreadStickRadius, query);
        }
    } else {
        // 糸に乗っていない場合（新規で糸に吸着するため）
        // ★ 修正：FindForwardThread や FindNearestThread を廃止し、FindTargetThread に変更！
        found = thread_->FindTargetThread(nextPos, currentMoveDir, kThreadStickRadius, query);
        if (!found) {
            found = thread_->FindTargetThread(translate_, currentMoveDir, kThreadStickRadius, query);
        }
    }

    if (!found) {
        onThread_ = false;
        IsCollisionSDF();
        return;
    }

    onThread_ = true;
    threadStart_ = query.startPoint;
    threadEnd_ = query.endPoint;

    // ThreadManagerに「移動補正ベクトル」と「端のフェード値(edgeFade)」を計算してもらう
    ThreadManager::ConstrainedMoveResult moveResult = thread_->CalculateConstrainedVelocity(nextPos, query);

    // 1. 横方向の補正を適用
    moveVel_.x += moveResult.velocityCorrection.x;
    moveVel_.z += moveResult.velocityCorrection.z;

    // 2. Y座標の沈み込み処理
    float targetY = query.closestPoint.y + threadOffsetY_;

    // ThreadManagerから貰った edgeFade を使ってY座標をブレンド
    translate_.y = threadBaseY_ + (targetY - threadBaseY_) * moveResult.edgeFade;

    // 3. 糸への重さの適用（※関数名を ApplyWeight に変更した想定）
    thread_->ApplyWeight(query.closestPoint, kThreadWeightRadius, kThreadWeight);
}