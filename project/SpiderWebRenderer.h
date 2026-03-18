#pragma once

#include <Vector2.h>
#include <Vector4.h>

#include <d3d12.h>
#include <wrl.h>
#include <vector>

class Camera;
struct SpiderWebData;

// 蜘蛛の巣の描画を専門に行うクラス
class SpiderWebRenderer {
private:
    // 頂点1つ分のデータ構造（シェーダーに送る形）
    struct VertexData {
        Vector4 position;
        Vector2 texcoord; // Typo修正: texcord -> texcoord
        Vector3 normal;
    };

    // インスタンス（蜘蛛の巣1個ずつ）ごとのデータ
    struct InstanceData {
        Matrix4x4 World;
    };

    // カメラの行列などを送るための定数バッファ用構造体
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    // マテリアル（色やライティング設定）のデータ
    struct MaterialData {
        Vector4 color;
        int32_t enableLighting;
        float padding[3]; // HLSLの16バイトアライメント規則に合わせるための余白
    };

public:
    ~SpiderWebRenderer();

    void Initialize(int maxWebs);
    void Update(const std::vector<SpiderWebData>& webs, Camera* camera);
    void Draw();

private:
    // --- 蜘蛛の巣の形状パラメータ ---
    static constexpr int kNumSpokes = 9;           // 放射状の糸の数（奇数で不規則感を出す）
    static constexpr int kNumRings = 5;            // 円状の糸の数
    static constexpr float kThreadThickness = 0.025f; // 糸の太さ
    static constexpr int kSegmentsPerArc = 4;      // 1区間の分割数（大きいほど滑らかな弧になる）
    static constexpr float kSagAmount = 0.05f;     // たわみの強さ
    static constexpr float kPi = 3.1415926535f;    // 円周率

    int maxWebs_ = 0;
    int verticesPerWeb_ = 0;   // 1個の巣を構成する頂点数
    int currentWebCount_ = 0;  // 現在描画すべき巣の数

    // --- DirectX12 リソース ---
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_ {};

    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* mappedInstanceData_ = nullptr; // Persistent Mapping用

    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjMatrixResource_;
    TransformationMatrix* mappedViewProjMatrix_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    uint32_t textureHandle_ = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU_ {};

private:
    // --- 初期化・頂点生成用のヘルパー関数 ---
    void PushLineQuad(VertexData* mappedVertexData, int& vIndex, const Vector3& p0, const Vector3& p1, float thickness, const Vector3& normal) const;
    void BuildFlatWeb(VertexData* mappedVertexData, int& vIndex, const Vector3& rightVec, const Vector3& upVec) const;

    void CreateVertexBuffer();
    void CreateInstanceBuffer();
    void CreateConstantBuffers();
    void RegisterPSO();
};