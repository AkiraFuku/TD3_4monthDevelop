#include "ThreadRenderer.h"

#include "Camera.h"
#include "DXCommon.h"
#include "MathFunction.h"
#include "TextureManager.h"

#include <cmath>

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
void ThreadRenderer::Initialize(int maxThreads, int nodesPerThread, float radius, int radialSegments) {
    radius_ = radius;
    radialSegments_ = radialSegments;

    // バッファの生成（処理を分割）
    CreateVertexBuffer(maxThreads, nodesPerThread);
    CreateConstantBuffers();

    // デフォルトテクスチャの読み込み
    textureHandle_ = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/white.png");
}

/// <summary>
/// 更新
/// </summary>
/// <param name="allThreadNodes">全糸の物理演算ノード配列</param>
/// <param name="camera">ビュー投影行列参照用カメラ</param>
void ThreadRenderer::Update(const std::vector<std::vector<PhysicsNode>>& allThreadNodes, Camera* camera) {
    // 頂点情報を計算しGPUへ転送
    UpdateVertices(allThreadNodes);

    // カメラ・ワールド行列の更新
    UpdateTransform(camera);
}

/// <summary>
/// 描画
/// </summary>
void ThreadRenderer::Draw() {
    if (currentVertexCount_ == 0) {
        return;
    }

    // マテリアルと行列をセット
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // テクスチャをセット
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_));

    // 頂点バッファと描画方式の設定
    DXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &vbView_);
    DXCommon::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画実行
    DXCommon::GetInstance()->GetCommandList()->DrawInstanced(currentVertexCount_, 1, 0, 0);
}

// ---------------------------------------------------------
// 内部処理関数 (初期化関連)
// ---------------------------------------------------------

/// <summary>
/// 合計頂点数に基づいた頂点バッファの作成
/// </summary>
void ThreadRenderer::CreateVertexBuffer(int maxThreads, int nodesPerThread) {
    // 全糸の合計頂点数を計算
    int maxSegmentsPerThread = nodesPerThread - 1;
    int verticesPerThread = maxSegmentsPerThread * radialSegments_ * 6;
    maxVertices_ = verticesPerThread * maxThreads;

    // キャッシュ用メモリの事前確保
    ringsCache_.reserve(nodesPerThread * (radialSegments_ + 1));

    // 動的頂点バッファの作成
    vertexBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * maxVertices_);

    // ビューの設定
    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * maxVertices_;
    vbView_.StrideInBytes = sizeof(VertexData);
}

/// <summary>
/// 行列およびマテリアル用定数バッファの作成
/// </summary>
void ThreadRenderer::CreateConstantBuffers() {
    // 行列バッファの作成とマッピング
    transformationMatrixResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpResource_));

    wvpResource_->WVP = Makeidentity4x4();
    wvpResource_->World = Makeidentity4x4();
    wvpResource_->WorldInverseTranspose = Makeidentity4x4();

    // マテリアルバッファの作成とマッピング
    materialResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // デフォルト値 (白・ライティング無効)
    materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData_->enableLighting = 0;
}

// ---------------------------------------------------------
// 内部処理関数 (更新関連)
// ---------------------------------------------------------

/// <summary>
/// 全糸のノードを走査して頂点バッファを書き換える
/// </summary>
void ThreadRenderer::UpdateVertices(const std::vector<std::vector<PhysicsNode>>& allThreadNodes) {
    VertexData* mappedData = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

    int vertexIndex = 0;

    // 各糸ごとにメッシュを生成
    for (const auto& nodes : allThreadNodes) {
        if (nodes.size() < 2) continue;

        GenerateThreadMesh(nodes, mappedData, vertexIndex);
    }

    vertexBuffer_->Unmap(0, nullptr);
    currentVertexCount_ = vertexIndex;
}

/// <summary>
/// 単一の糸のメッシュ（チューブ状）を生成してバッファへ書き込む
/// </summary>
void ThreadRenderer::GenerateThreadMesh(const std::vector<PhysicsNode>& nodes, VertexData* mappedData, int& vertexIndex) {
    // メモリ再確保を起こさずに中身のみクリア
    ringsCache_.clear();

    // 1. ノードごとにリング状の頂点座標を計算
    for (size_t i = 0; i < nodes.size(); ++i) {
        Vector3 forward;
        if (i < nodes.size() - 1) {
            forward = nodes[i + 1].currentPos - nodes[i].currentPos;
        } else {
            forward = nodes[i].currentPos - nodes[i - 1].currentPos;
        }
        forward = Normalize(forward);

        // 上方向ベクトルの決定 (ジンバルロック対策)
        Vector3 up = {0.0f, 1.0f, 0.0f};
        if (std::abs(forward.y) > 0.99f) {
            up = {1.0f, 0.0f, 0.0f};
        }

        Vector3 right = Normalize(Cross(up, forward));
        Vector3 trueUp = Normalize(Cross(forward, right));

        // 断面の頂点を生成してキャッシュに追加
        for (int j = 0; j <= radialSegments_; ++j) {
            float angle = (static_cast<float>(j) / radialSegments_) * 3.14159265f * 2.0f;
            Vector3 offset = (right * std::cos(angle)) + (trueUp * std::sin(angle));
            Vector3 normal = Normalize(offset);
            Vector3 pos = nodes[i].currentPos + (normal * radius_);

            VertexData v;
            v.position = {pos.x, pos.y, pos.z, 1.0f};
            v.normal = normal;
            v.texcord = {
                static_cast<float>(j) / radialSegments_,
                static_cast<float>(i) / (nodes.size() - 1)
            };
            ringsCache_.push_back(v);
        }
    }

    // 2. キャッシュからポリゴンを構築しGPUバッファへ書き込み
    int ringSize = radialSegments_ + 1;
    for (size_t i = 0; i < nodes.size() - 1; ++i) {
        for (int j = 0; j < radialSegments_; ++j) {
            // バッファ超過防止
            if (vertexIndex + 6 > maxVertices_) break;

            VertexData a = ringsCache_[i * ringSize + j];
            VertexData b = ringsCache_[i * ringSize + j + 1];
            VertexData c = ringsCache_[(i + 1) * ringSize + j];
            VertexData d = ringsCache_[(i + 1) * ringSize + j + 1];

            // 2つの三角形で四角形ポリゴンを構成
            mappedData[vertexIndex++] = a;
            mappedData[vertexIndex++] = b;
            mappedData[vertexIndex++] = c;
            mappedData[vertexIndex++] = c;
            mappedData[vertexIndex++] = b;
            mappedData[vertexIndex++] = d;
        }
    }
}

/// <summary>
/// ワールド行列およびWVP行列の更新
/// </summary>
void ThreadRenderer::UpdateTransform(Camera* camera) {
    if (!camera) return;

    // 頂点はすでにワールド座標のため、単位行列を設定
    Matrix4x4 worldMatrix = Makeidentity4x4();

    // カメラ行列を乗算してWVP行列を作成
    wvpResource_->WVP = Multiply(worldMatrix, camera->GetViewProtectionMatrix());
    wvpResource_->World = worldMatrix;
    wvpResource_->WorldInverseTranspose = worldMatrix;
}