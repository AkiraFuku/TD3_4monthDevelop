#include "PSOManager.h" // ファイル名変更に合わせてインクルードも変更
#include "DXCommon.h"
#include "Logger.h"
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h> // IDxcBlobのため


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

std::unique_ptr<PipelineStateManager> PipelineStateManager::instance_ = nullptr;

PipelineStateManager* PipelineStateManager::GetInstance() {
    if (!instance_) {
        instance_.reset(new PipelineStateManager());
    }
    return instance_.get();
}



void PipelineStateManager::Initialize() {
    pipelineStateCache_.clear();
    rootSignatureCache_.clear();
    configurations_.clear();
    shaderCache_.clear();
}

void PipelineStateManager::Finalize() {
    Initialize();
}

void PipelineStateManager::RegisterConfiguration(const std::string& name, const PipelineStateConfig& configuration) {
    configurations_[name] = configuration;
}

D3D12_STATIC_SAMPLER_DESC PipelineStateManager::CreateDefaultStaticSamplerDescription()
{

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    return sampler;
}

const PipelineStateSet& PipelineStateManager::GetOrCreatePipelineState(const std::string& name, BlendMode blend, FillMode fill) {
    CacheKey key{ name, blend, fill };

    // キャッシュに存在すればそれを返す
    if (pipelineStateCache_.contains(key)) {
        return pipelineStateCache_[key];
    }

    // 存在しなければ新しく生成してキャッシュに登録
    CreatePipelineState(name, blend, fill);
    return pipelineStateCache_.at(key);
}


// -------------------------------------------------------------------------
// シェーダー管理（重複コンパイル防止）
// -------------------------------------------------------------------------
void PipelineStateManager::EnsureShaders(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outVertexShader, Microsoft::WRL::ComPtr<IDxcBlob>& outPixelShader) {
    // 既にキャッシュにあればそれを返す
    if (shaderCache_.contains(name)) {
        outVertexShader = shaderCache_[name].vertexShader;
        outPixelShader = shaderCache_[name].ps;
        return;
    }



    auto dxCommon = DirectXCommon::GetInstance();

    configurations_[name];

    ComPtr<IDxcBlob> vs = dxCommon->CompileShader(configurations_[name].vertexShaderPath, L"vs_6_0");
    ComPtr<IDxcBlob> ps = dxCommon->CompileShader(configurations_[name].pixelShaderPath, L"ps_6_0");



    assert(vs && ps);

    // キャッシュに保存
    shaderCache_[name] = { vs, ps };

    outVertexShader = vs;
    outPixelShader = ps;


}

void PipelineStateManager::CreatePipelineState(const std::string& name, BlendMode blendMode, FillMode fillMode) {
    auto device = DirectXCommon::GetInstance()->GetDevice();
    const auto& config = configurations_.at(name);

    // 1. RootSignature のキャッシュ確認と生成
    if (!rootSignatureCache_.contains(name)) {
        assert(config.rootSignatureGenerator && "RootSignatureGenerator is null");
        rootSignatureCache_[name] = config.rootSignatureGenerator();
    }
    auto rootSignature = rootSignatureCache_[name];

    // 2. InputLayout の取得
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    if (config.inputLayoutGenerator) {
        inputElements = config.inputLayoutGenerator();
    }

    // 3. Shader の取得
    ComPtr<IDxcBlob> vsBlob, psBlob;
    EnsureShaders(name, vsBlob, psBlob);

    // 4. PSO構築
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    // 各種ステートの設定
    psoDesc.BlendState = CreateBlendDesc(blendMode);

    // Rasterizer State
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = (fillMode == FillMode::kWireFrame) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = config.cullMode;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // DepthStencil State (Configからの値を優先)
    psoDesc.DepthStencilState = config.depth;
    psoDesc.DepthStencilState.DepthEnable = config.depthEnable;
    psoDesc.DepthStencilState.DepthWriteMask = config.depthWriteMask;
    // 深度比較関数が設定されていない場合のデフォルト
    if (psoDesc.DepthStencilState.DepthFunc == 0) {
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }

    // フォーマット設定 (環境に合わせて適宜変更)
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.NumRenderTargets = 1;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    PipelineStateSet psoSet;
    psoSet.rootSignature = rootSignature;
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoSet.pipelineState));
    assert(SUCCEEDED(hr) && "Failed to create Pipeline State");

    pipelineStateCache_[{name, blendMode, fillMode}] = psoSet;
}

D3D12_BLEND_DESC PipelineStateManager::CreateBlendDesc(BlendMode mode) {
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = (mode != BlendMode::None);

    // 共通初期値
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    switch (mode) {
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
    default:
        break;
    }
    return blendDesc;
}