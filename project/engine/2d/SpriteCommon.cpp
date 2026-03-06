#include "SpriteCommon.h"
#include "Logger.h"
#include <cassert>
#include "DXCommon.h"
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

    settings_.pipelineName = "Sprite_Default";
    settings_.vsPath = L"resources/shaders/Sprite/Sprite.vs.hlsl";
    settings_.psPath = L"resources/shaders/Sprite/Sprite.ps.hlsl";
    settings_.blendMode = BlendMode::Normal;
    settings_.cullMode = D3D12_CULL_MODE_BACK;

    settings_.rootSigBuilder = [](std::vector<D3D12_ROOT_PARAMETER>& params, std::vector<D3D12_STATIC_SAMPLER_DESC>& samplers) {

        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplers.push_back(sampler);

        D3D12_DESCRIPTOR_RANGE descRangeTexture[1]{};
        descRangeTexture[0].BaseShaderRegister = 0; // t0
        descRangeTexture[0].NumDescriptors = 1;
        descRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        params.resize(3);
        // Enum定義 (可読性のため)
        enum {
            kMaterial, kTransform, kTexture,
        };
        // 0. Material (CBV b0, Pixel)
        params[kMaterial].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[kMaterial].Descriptor.ShaderRegister = 0;
        params[kMaterial].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // 1. Transform (CBV b0, Vertex)
        params[kTransform].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[kTransform].Descriptor.ShaderRegister = 1;
        params[kTransform].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // 2. Texture (Table t0, Pixel)
        params[kTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[kTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        params[kTexture].DescriptorTable.pDescriptorRanges = descRangeTexture;
        params[kTexture].DescriptorTable.NumDescriptorRanges = 1;

        };


    //PsoProperty pso={PipelineType::Sprite,BlendMode::None,FillMode::kSolid};
    PsoSet PSO = PSOManager::GetInstance()->GetPsoSet(settings_);
    graphicsPipelineState_ = PSO.pipelineState;
    rootSignature_ = PSO.rootSignature;

    //CreatePSO();

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

