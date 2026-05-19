#pragma once

#include "PhysicsNode.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include <d3d12.h>
#include <wrl.h>
#include <vector>

class Camera;

class ThreadRenderer {
private:
    // ---------------------------------------------------------
    // 構造体
    // ---------------------------------------------------------

    // 頂点データ
    struct VertexData {
        Vector4 position;
        Vector2 texcord;
        Vector3 normal;
    };

    // 変換行列データ
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    // マテリアルデータ
    struct MaterialData {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
    };

public:
    // ---------------------------------------------------------
    // 公開関数
    // ---------------------------------------------------------

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="maxThreads">描画する糸の最大本数</param>
    /// <param name="nodesPerThread">1本あたりの最大ノード数</param>
    /// <param name="radius">糸の太さ</param>
    /// <param name="radialSegments">断面の分割数</param>
    void Initialize(int maxThreads, int nodesPerThread, float radius = 0.02f, int radialSegments = 6);

    /// <summary>
    /// 更新
    /// </summary>
    /// <param name="allThreadNodes">全糸の物理演算ノード配列</param>
    /// <param name="camera">ビュー投影行列参照用カメラ</param>
    void Update(const std::vector<std::vector<PhysicsNode>>& allThreadNodes, Camera* camera);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// 糸の色を設定する
    /// </summary>
    void SetColor(const Vector4& color);

private:
    // ---------------------------------------------------------
    // 内部処理関数
    // ---------------------------------------------------------

    /// <summary>
    /// 合計頂点数に基づいた頂点バッファの作成
    /// </summary>
    void CreateVertexBuffer(int maxThreads, int nodesPerThread);

    /// <summary>
    /// 行列およびマテリアル用定数バッファの作成
    /// </summary>
    void CreateConstantBuffers();

    /// <summary>
    /// 全糸のノードを走査して頂点バッファを書き換える
    /// </summary>
    void UpdateVertices(const std::vector<std::vector<PhysicsNode>>& allThreadNodes);

    /// <summary>
    /// 単一の糸のメッシュ（チューブ状）を生成してバッファへ書き込む
    /// </summary>
    void GenerateThreadMesh(const std::vector<PhysicsNode>& nodes, VertexData* mappedData, int& vertexIndex);

    /// <summary>
    /// ワールド行列およびWVP行列の更新
    /// </summary>
    void UpdateTransform(Camera* camera);

private:
    // ---------------------------------------------------------
    // メンバ変数
    // ---------------------------------------------------------

    // 形状パラメータ
    float radius_ = 0.02f;
    int radialSegments_ = 6;

    // 描画管理
    int maxVertices_ = 0;        // 確保した最大頂点数
    int currentVertexCount_ = 0; // 今フレームの描画頂点数
    uint32_t textureHandle_ = 0; // テクスチャ

    // 計算用キャッシュ（毎フレームのメモリ確保を防止）
    std::vector<VertexData> ringsCache_;

    // DirectXリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_ {};

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* wvpResource_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    MaterialData* materialData_ = nullptr;
};