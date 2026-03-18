#define NOMINMAX
#include "SpiderWebRenderer.h"

#include "SpiderWebManager.h" 
#include "Camera.h"
#include "DXCommon.h"
#include "TextureManager.h"
#include "MathFunction.h"
#include "DrawFunction.h"
#include "PSOManager.h"
#include "SrvManager.h"

#include <cmath>
#include <random>

SpiderWebRenderer::~SpiderWebRenderer() {
    if (instanceBuffer_ && mappedInstanceData_) {
        instanceBuffer_->Unmap(0, nullptr);
    }
    if (viewProjMatrixResource_ && mappedViewProjMatrix_) {
        viewProjMatrixResource_->Unmap(0, nullptr);
    }
}

void SpiderWebRenderer::Initialize(int maxWebs) {
    maxWebs_ = maxWebs;

    CreateVertexBuffer();
    CreateInstanceBuffer();
    CreateConstantBuffers();

    textureHandle_ = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/white.png");
    RegisterPSO();
}

void SpiderWebRenderer::Update(const std::vector<SpiderWebData>& webs, Camera* camera) {
    currentWebCount_ = std::min(static_cast<int>(webs.size()), maxWebs_);

    // --- 各蜘蛛の巣のWorld行列を計算してインスタンスバッファに書き込む ---
    for (int i = 0; i < currentWebCount_; ++i) {
        Matrix4x4 scaleMat = MakeScaleMatrix({webs[i].scale, webs[i].scale, webs[i].scale});
        Matrix4x4 transMat = MakeTranslateMatrix(webs[i].position);
        mappedInstanceData_[i].World = Multiply(scaleMat, transMat);
    }

    // --- カメラ情報の更新 ---
    if (camera && mappedViewProjMatrix_) {
        mappedViewProjMatrix_->WVP = camera->GetViewProtectionMatrix();
        mappedViewProjMatrix_->World = Makeidetity4x4(); // 既存関数そのまま
        mappedViewProjMatrix_->WorldInverseTranspose = Makeidetity4x4();
    }
}

void SpiderWebRenderer::Draw() {
    if (currentWebCount_ == 0) return;
    auto cmdList = DXCommon::GetInstance()->GetCommandList();

    const PsoSet& pso = PSOManager::GetInstance()->GetPso("SpiderWeb", BlendMode::Normal);
    cmdList->SetGraphicsRootSignature(pso.rootSignature.Get());
    cmdList->SetPipelineState(pso.pipelineState.Get());

    cmdList->IASetVertexBuffers(0, 1, &vbView_);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Root parameters
    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress()); // b0
    cmdList->SetGraphicsRootConstantBufferView(1, viewProjMatrixResource_->GetGPUVirtualAddress()); // b1
    cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_)); // t0
    cmdList->SetGraphicsRootDescriptorTable(3, instanceSrvHandleGPU_); // t1

    cmdList->DrawInstanced(verticesPerWeb_, currentWebCount_, 0, 0);
}

// =========================================================================
// 頂点生成用のプライベートヘルパー関数
// =========================================================================

void SpiderWebRenderer::PushLineQuad(VertexData* mappedVertexData, int& vIndex, const Vector3& p0, const Vector3& p1, float thickness, const Vector3& normal) const {
    Vector3 dir = Subtract(p1, p0);
    if (Length(dir) < 0.0001f) return;

    dir = Normalize(dir);
    Vector3 right = Normalize(Cross(dir, normal));
    right = Multiply(thickness * 0.5f, right);

    Vector3 tl = Subtract(p0, right);
    Vector3 tr = Add(p0, right);
    Vector3 bl = Subtract(p1, right);
    Vector3 br = Add(p1, right);

    Vector2 dummyUV = {0.0f, 0.0f};

    // --- 表面 (時計回り) ---
    mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, normal};
    mappedVertexData[vIndex++] = {{tr.x, tr.y, tr.z, 1.0f}, dummyUV, normal};
    mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, normal};
    mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, normal};
    mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, normal};
    mappedVertexData[vIndex++] = {{bl.x, bl.y, bl.z, 1.0f}, dummyUV, normal};

    // --- 裏面 (反時計回り) ---
    Vector3 invertedNormal = {-normal.x, -normal.y, -normal.z};
    mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, invertedNormal};
    mappedVertexData[vIndex++] = {{bl.x, bl.y, bl.z, 1.0f}, dummyUV, invertedNormal};
    mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, invertedNormal};
    mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, invertedNormal};
    mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, invertedNormal};
    mappedVertexData[vIndex++] = {{tr.x, tr.y, tr.z, 1.0f}, dummyUV, invertedNormal};
}

void SpiderWebRenderer::BuildFlatWeb(VertexData* mappedVertexData, int& vIndex, const Vector3& rightVec, const Vector3& upVec) const {
    std::mt19937 randEngine(12345); // 固定シード
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    Vector3 center = {0.0f, 0.0f, 0.0f};
    Vector3 normal = Cross(rightVec, upVec);

    // 1. 各放射状の糸（Spokes）の角度決定
    std::vector<float> spokeAngles(kNumSpokes);
    for (int i = 0; i < kNumSpokes; ++i) {
        float baseAngle = (kPi * 2.0f / kNumSpokes) * i;
        float angleJitter = dist(randEngine) * ((kPi * 2.0f) / kNumSpokes * 0.3f);
        spokeAngles[i] = baseAngle + angleJitter;
    }

    // 2. 各円状の糸（Rings）のベース半径決定
    std::vector<float> ringBaseRadii(kNumRings);
    float baseStep = 1.0f / kNumRings;
    for (int r = 0; r < kNumRings; ++r) {
        float normalRadius = static_cast<float>(r + 1) * baseStep;
        float jitter = dist(randEngine) * (baseStep * 0.4f);
        ringBaseRadii[r] = normalRadius + jitter;
    }

    // 3. 全交点（ノード）の座標計算
    std::vector<std::vector<Vector3>> nodes(kNumRings, std::vector<Vector3>(kNumSpokes));
    for (int r = 0; r < kNumRings; ++r) {
        for (int i = 0; i < kNumSpokes; ++i) {
            float finalRadius = ringBaseRadii[r] + dist(randEngine) * (baseStep * 0.2f);
            float c = std::cos(spokeAngles[i]) * finalRadius;
            float s = std::sin(spokeAngles[i]) * finalRadius;

            nodes[r][i] = Add(center, Add(Multiply(c,rightVec), Multiply(s, upVec)));
        }
    }

    // 4. 放射状の糸(Spokes)を描画
    for (int i = 0; i < kNumSpokes; ++i) {
        PushLineQuad(mappedVertexData, vIndex, center, nodes[kNumRings - 1][i], kThreadThickness, normal);
    }

    // 5. 円状の糸(Rings)を描画（たわみ追加）
    for (int r = 0; r < kNumRings; ++r) {
        for (int i = 0; i < kNumSpokes; ++i) {
            int next_i = (i + 1) % kNumSpokes;
            Vector3 p0 = nodes[r][i];
            Vector3 p1 = nodes[r][next_i];
            Vector3 prevPoint = p0;

            for (int j = 1; j <= kSegmentsPerArc; ++j) {
                float t = static_cast<float>(j) / kSegmentsPerArc;

                // 線形補間
                Vector3 lin = Add(p0, Multiply(t, Subtract(p1, p0)));

                // たわみ計算
                float weight = 4.0f * t * (1.0f - t);
                Vector3 dirToCenter = Normalize(Subtract(center, lin)); // 中心へ向かうベクトル
                float currentSag = kSagAmount * (static_cast<float>(r + 1) / kNumRings);

                // オフセット適用
                Vector3 arcP = Add(lin, Multiply(currentSag * weight, dirToCenter));

                PushLineQuad(mappedVertexData, vIndex, prevPoint, arcP, kThreadThickness * 0.8f, normal);
                prevPoint = arcP;
            }
        }
    }
}

void SpiderWebRenderer::CreateVertexBuffer() {
    verticesPerWeb_ = ((kNumSpokes * 12) + (kNumSpokes * kNumRings * kSegmentsPerArc * 12)) * 3 + 100;

    vertexBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * verticesPerWeb_);
    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * verticesPerWeb_;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* mappedVertexData = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));
    if (!mappedVertexData) return; // マップ失敗時の安全対策

    int vIndex = 0;

    // 3つの平面を組み合わせて球体状にする
    BuildFlatWeb(mappedVertexData, vIndex, {0.707f, 0.0f, 0.707f}, {0.0f, 1.0f, 0.0f});
    BuildFlatWeb(mappedVertexData, vIndex, {0.707f, 0.0f, -0.707f}, {0.0f, 1.0f, 0.0f});
    BuildFlatWeb(mappedVertexData, vIndex, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

    vertexBuffer_->Unmap(0, nullptr);
    verticesPerWeb_ = vIndex;
}

void SpiderWebRenderer::CreateInstanceBuffer() {
    instanceBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(InstanceData) * maxWebs_);
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_));

    uint32_t srvIndex = SrvManager::GetInstance()->AllocateSRV();
    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
        srvIndex,
        instanceBuffer_.Get(),
        maxWebs_,
        sizeof(InstanceData)
    );

    instanceSrvHandleGPU_ = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
}

void SpiderWebRenderer::CreateConstantBuffers() {
    viewProjMatrixResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    viewProjMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedViewProjMatrix_));

    materialResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));
    MaterialData* matData = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&matData));
    if (matData) {
        matData->color = {1.0f, 1.0f, 1.0f, 1.0f};
        matData->enableLighting = 0;
    }
    materialResource_->Unmap(0, nullptr);
}

void SpiderWebRenderer::RegisterPSO() {
    PsoConfig config {};

    config.vsPath = L"resources/shaders/SpiderWeb/SpiderWeb.VS.hlsl";
    config.psPath = L"resources/shaders/SpiderWeb/SpiderWeb.PS.hlsl";

    config.inputLayoutGenerator = []() {
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };
        };

    config.rootSignatureGenerator = []() {
        auto device = DXCommon::GetInstance()->GetDevice();
        CD3DX12_ROOT_PARAMETER rootParameters[4] {};

        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // b1

        CD3DX12_DESCRIPTOR_RANGE texRange;
        texRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[2].InitAsDescriptorTable(1, &texRange, D3D12_SHADER_VISIBILITY_ALL); // t0

        CD3DX12_DESCRIPTOR_RANGE instRange;
        instRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[3].InitAsDescriptorTable(1, &instRange, D3D12_SHADER_VISIBILITY_ALL); // t1

        auto sampler = PSOManager::GetInstance()->StaticSamplers();
        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
        device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
        return rootSig;
        };

    config.cullMode = D3D12_CULL_MODE_NONE;

    PSOManager::GetInstance()->RegisterPsoGenerator("SpiderWeb", config);
}