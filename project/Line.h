#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <wrl.h>
#include <d3d12.h>





class Line3d {
public:
    struct VertexData {
        Vector3 position;
        Vector4 color;
    };

    struct TransformationMatrix {
        Matrix4x4 WVP;
    };

    void Initialize();
    void Update(const Vector3& start, const Vector3& end, const Vector4& color);
    void Draw();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    VertexData* vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;
};