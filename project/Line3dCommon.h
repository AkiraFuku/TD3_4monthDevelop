#pragma once
#include "d3d12.h"
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include "DXCommon.h"
#include "Camera.h"

class Line3dCommon
{
public:
  // シングルトン化
    static Line3dCommon* GetInstance();
    void Finalize();

    void Initialize();

   
    void Line3dCommonDraw();
      void SetDefaultCamera(Camera* camera) {
        defaultCamera_ = camera;
    }
    Camera* GetDefaultCamera()const {
        return defaultCamera_;
    }
    // 静的メンバ変数の宣言
    static std::unique_ptr<Line3dCommon> instance;
      friend struct std::default_delete<Line3dCommon>;
private:
    // シングルトンパターン
    Line3dCommon() = default;
    ~Line3dCommon() = default;
    Line3dCommon(const Line3dCommon&) = delete;
    Line3dCommon& operator=(const Line3dCommon&) = delete;
    HRESULT hr_;

    

    //ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    //グラフィックパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    Camera* defaultCamera_ = nullptr;

};

