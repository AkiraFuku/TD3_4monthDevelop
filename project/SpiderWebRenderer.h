#pragma once

#include <Vector2.h>
#include <Vector4.h>

#include <d3d12.h>
#include <wrl.h>
#include <vector>

class Camera;
struct SpiderWebData;

class SpiderWebRenderer {
private:
    struct VertexData {
        Vector4 position;
        Vector2 texcord; // 今回は使いませんが形式上残します
        Vector3 normal;
    };

    // ★ 蜘蛛の巣1個ずつの位置・スケールを保持する構造体
    struct InstanceData {
        Matrix4x4 World;
    };

    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    struct MaterialData {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
    };

public:
    void Initialize(int maxWebs);
    void Update(const std::vector<SpiderWebData>& webs, Camera* camera);
    void Draw();

private:
    int maxWebs_ = 0;
    int verticesPerWeb_ = 0; // 1個の巣の頂点数
    int currentWebCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_ {};

    // ★ インスタンスバッファ（座標一覧）
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* mappedInstanceData_ = nullptr;

    // カメラのViewProjection行列だけを送るバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjMatrixResource_;
    TransformationMatrix* mappedViewProjMatrix_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    MaterialData* materialData_ = nullptr;
};