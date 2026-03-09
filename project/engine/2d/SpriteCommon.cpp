#include "SpriteCommon.h"
#include "Logger.h"
#include <cassert>
#include "DXCommon.h"
#include "PSOManager.h"
// 静的メンバ変数の初期化
std::unique_ptr<SpriteCommon> SpriteCommon::instance = nullptr;

SpriteCommon* SpriteCommon::GetInstance() {
    if (instance == nullptr) {
        instance.reset(new SpriteCommon());
    }
    return instance.get();
}

void SpriteCommon::Finalize() {

    instance.reset();
}
void SpriteCommon::Initialize()
{
   auto psoManager = PSOManager::GetInstance();

    psoManager->RegisterPsoGenerator("Sprite", []() {
        PsoConfig config;
        config.vsPath = L"resources/shaders/Sprite/Sprite.vs.hlsl";
        config.psPath = L"resources/shaders/Sprite/Sprite.ps.hlsl";

        // インプットレイアウト
    /* config.inputElements = {
         { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT },
        };*/

        // ルートパラメータ (Sprite.cpp の Draw に合わせる)
        static D3D12_DESCRIPTOR_RANGE descRange[1]{};
        descRange[0].BaseShaderRegister = 0; // t0
        descRange[0].NumDescriptors = 1;
        descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        config.rootParameters.resize(3);
        // 0: Material (b0)
        config.rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        config.rootParameters[0].Descriptor.ShaderRegister = 0;
        config.rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        // 1: TransformationMatrix (b1)
        config.rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        config.rootParameters[1].Descriptor.ShaderRegister = 1;
        config.rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        // 2: Texture (t0)
        config.rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        config.rootParameters[2].DescriptorTable.pDescriptorRanges = descRange;
        config.rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
        config.rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 共通サンプラーの設定（必要に応じて PsoConfig に追加）
        static D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        config.samplers.push_back(sampler);

        // スプライトは背面カリングをしないことが多い
        config.cullMode = D3D12_CULL_MODE_NONE;
        config.depthEnable = false; // 2DなのでDepthは切る設定

        return config;
    });

    // 初期化時にキャッシュを作っておく
    auto& pso = psoManager->GetPso("Sprite", BlendMode::Normal);
    rootSignature_ = pso.rootSignature;
    graphicsPipelineState_ = pso.pipelineState;

}

void SpriteCommon::SpriteCommonDraw()
{
    // RootSignatureの設定
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
    //  //PSOの設定
    DXCommon::GetInstance()->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());

    //プリミティブトポロジーのセット
    DXCommon::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

