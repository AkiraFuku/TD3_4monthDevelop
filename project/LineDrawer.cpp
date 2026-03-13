#include "LineDrawer.h"
#include "PSOManager.h"
#include "DXCommon.h"
void LineDrawer::Initialize() {
    // 1. PSOManagerに線用PSOを登録
    PsoConfig config;
    config.vsPath = L"Resources/shaders/Line.VS.hlsl";
    config.psPath = L"Resources/shaders/Line.PS.hlsl";
    config.topologyType = TopologyType::kLine; // トポロジーを線に設定

    // ルートシグネチャ生成関数の登録
    config.rootSignatureGenerator = []() {
        // 定数バッファ(ViewProjection)等を含むRootSignatureを作成して返す
        return CreateLineRootSignature();
        };

    // インプットレイアウトの登録
    config.inputLayoutGenerator = []() {
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        };

    PSOManager::GetInstance()->RegisterPsoGenerator("LineDrawer", config);

    // 2. 頂点バッファの作成 (省略: D3D12の一般的なBuffer作成)
}

void LineDrawer::AddLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
}
void LineDrawer::Draw() {
    if (vertexCount_ == 0) return;

    // PSOManagerからPSOセットを取得
    auto& psoSet = PSOManager::GetInstance()->GetPso("LineDrawer", BlendMode::Normal);
    auto commandList = DXCommon::GetInstance()->GetCommandList();

    commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
    commandList->SetPipelineState(psoSet.pipelineState.Get());

    // 線描画に必須の設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &vbView_);

    // 定数バッファのセットなどを経て描画
    commandList->DrawInstanced(vertexCount_, 1, 0, 0);
}

void LineDrawer::Reset() {
    vertexCount_ = 0;
}
void LineDrawer::CreateVertexBuffer() {
    //頂点リソースの作成
    vertBuff_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(LineVertex) );
    //頂点バッファビューの設定
    vbView_.BufferLocation =
        vertBuff_.Get()->GetGPUVirtualAddress();
    vbView_.SizeInBytes = UINT(sizeof(LineVertex) );
    vbView_.StrideInBytes = sizeof(LineVertex);
    vertBuff_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_));

    //頂点データの転送
    memcpy(mappedVertices_,  sizeof(LineVertex) );
}