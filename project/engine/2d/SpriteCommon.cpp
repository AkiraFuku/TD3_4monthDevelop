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
    PsoProperty pso={PipelineType::Sprite,BlendMode::None,FillMode::kSolid};
    PsoSet psoset=PSOManager::GetInstance()->GetPsoSet(pso);
    graphicsPipelineState_=psoset.pipelineState;
    rootSignature_=psoset.rootSignature;

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

