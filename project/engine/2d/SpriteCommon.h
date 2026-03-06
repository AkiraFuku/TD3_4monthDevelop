#pragma once
#include "d3d12.h"
#include <wrl/client.h>
#include <cstdint>
#include "PSOManager.h"

#include <memory>
class SpriteCommon
{
public:
  // シングルトン化
    static SpriteCommon* GetInstance();
    void Finalize();

    void Initialize();

   PsoSettings GetPsoSettings() const {
        return settings_;
   }
    void SpriteCommonDraw();

    static std::unique_ptr<SpriteCommon> instance;
      friend struct std::default_delete<SpriteCommon>;
private:
    // シングルトンパターン
    SpriteCommon() = default;
    ~SpriteCommon() = default;
    SpriteCommon(const SpriteCommon&) = delete;
    SpriteCommon& operator=(const SpriteCommon&) = delete;
    HRESULT hr_;

    
    
    PsoSettings settings_;
    //ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
    //グラフィックパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;


};

