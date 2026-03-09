#include "PSOManager.h"
#include "DXCommon.h"
#include <cassert>

using namespace Microsoft::WRL;

std::unique_ptr<PSOManager> PSOManager::instance_ = nullptr;

PSOManager* PSOManager::GetInstance() {
    if (!instance_) instance_.reset(new PSOManager());
    return instance_.get();
}
void PSOManager::Initialize() {
    psoCache_.clear();
    rootSigCache_.clear();
    generators_.clear();
    configCache_.clear();
}

void PSOManager::Finalize() {
    psoCache_.clear();
    rootSigCache_.clear();
    generators_.clear();
    configCache_.clear();
}


void PSOManager::RegisterPsoGenerator(const std::string& name, PsoGenerator generator) {
    generators_[name] = generator;
}

const PsoSet& PSOManager::GetPso(const std::string& name, BlendMode blend, FillMode fill) {
    CacheKey key{ name, blend, fill };
    if (psoCache_.contains(key)) {
        return psoCache_[key];
    }
    CreatePso(name, blend, fill);
    return psoCache_.at(key);
}

void PSOManager::CreatePso(const std::string& name, BlendMode blend, FillMode fill) {

    assert(generators_.contains(name) && "PSO Generator not registered!");



    // 1. レシピ（Config）を取得
    PsoConfig config = generators_[name]();
    auto device = DXCommon::GetInstance()->GetDevice();
   /* assert(config.inputElements.size() > 0);*/
    // 2. RootSignatureの生成・取得
    if (!rootSigCache_.contains(name)) {
      /*  D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rsDesc.pParameters = config.rootParameters.data();
        rsDesc.NumParameters = (UINT)config.rootParameters.size();
        rsDesc.pStaticSamplers = config.samplers.data();
        rsDesc.NumStaticSamplers = (UINT)config.samplers.size();*/

        ComPtr<ID3DBlob> sigBlob, errorBlob;
        D3D12SerializeRootSignature(&config.rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob);
        device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSigCache_[name]));
    }

     //InputLayoutの設定
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    //
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    //
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);
    // 3. シェーダーコンパイル
    auto dx = DXCommon::GetInstance();
    auto vs = dx->CompileShader(config.vsPath.c_str(), L"vs_6_0");
    auto ps = dx->CompileShader(config.psPath.c_str(), L"ps_6_0");

    // 4. PSO構築
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSigCache_[name].Get();
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.BlendState = CreateBlendDesc(blend);

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = config.cullMode;
    psoDesc.RasterizerState.FillMode = (fill == FillMode::kWireFrame) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = config.depthEnable;
    psoDesc.DepthStencilState.DepthWriteMask = config.depthWriteMask;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    PsoSet psoSet;
    psoSet.rootSignature = rootSigCache_[name];
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoSet.pipelineState));
    assert(SUCCEEDED(hr));

    psoCache_[{name, blend, fill}] = psoSet;
}

D3D12_BLEND_DESC PSOManager::CreateBlendDesc(BlendMode mode) {
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    // 共通初期値
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    switch (mode) {
    case BlendMode::None:
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case BlendMode::Normal:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Add:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Subtract:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        break;
    case BlendMode::Multiply:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Screen:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    }
    return blendDesc;
}