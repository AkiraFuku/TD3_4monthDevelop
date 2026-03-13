#pragma once
#include <vector>
#include <string>
#include <wrl/client.h>
#include <Vector3.h>
#include <Vector4.h>
#include <d3d12.h>
class LineDrawer {
public:
    void Initialize();

    // 線を追加（1フレームごとにクリアして使う想定）
    void AddLine(const Vector3& start, const Vector3& end, const Vector4& color);

    // 描画実行
    void Draw();

    void Reset(); // フレームの開始時に頂点数をリセット

private:
    struct LineVertex {
        Vector3 pos;
        Vector4 color;
    };

    static const uint32_t kMaxLines = 10000; // 最大線数
    static const uint32_t kMaxVertices = kMaxLines * 2;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    LineVertex* mappedVertices_ = nullptr;
    void CreateVertexBuffer();
    uint32_t vertexCount_ = 0;
};

