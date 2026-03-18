#include "SpiderWebRenderer.h"

#include "SpiderWebManager.h" // SpiderWebDataの定義用
#include "Camera.h"
#include "DXCommon.h"
#include "TextureManager.h"
#include "MathFunction.h"

void SpiderWebRenderer::Initialize(int maxWebs) {
    maxWebs_ = maxWebs;

    // ==========================================
    // 1. 「究極に滑らかで最強の1個」を生成する
    // ==========================================
    // 糸の数を16本、リングを8重にする（ガタガタがほぼ消えます）
    int numSpokes = 16;
    int numRings = 8;
    float threadThickness = 0.01f; // 少し細めにすると綺麗

    // バッファサイズの計算（大体2500頂点くらいになります）
    verticesPerWeb_ = (numSpokes * 12) + (numSpokes * numRings * 12);
    verticesPerWeb_ *= 2; // X字に2枚交差させるので2倍
    verticesPerWeb_ += 100; // 余裕を持たせる

    vertexBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * verticesPerWeb_);
    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * verticesPerWeb_;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* mappedVertexData = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));
    int vIndex = 0;

    // 線の生成関数（前と同じ）
    auto PushLineQuad = [&](const Vector3& p0, const Vector3& p1, float thickness, const Vector3& normal) {
        
            Vector3 dir = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (len < 0.0001f) return;
            dir = {dir.x / len, dir.y / len, dir.z / len};

            Vector3 right = {
                dir.y * normal.z - dir.z * normal.y,
                dir.z * normal.x - dir.x * normal.z,
                dir.x * normal.y - dir.y * normal.x
            };
            float rLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
            if (rLen > 0.0001f) { right = {right.x / rLen, right.y / rLen, right.z / rLen}; }

            right = {right.x * thickness * 0.5f, right.y * thickness * 0.5f, right.z * thickness * 0.5f};

            Vector3 tl = {p0.x - right.x, p0.y - right.y, p0.z - right.z};
            Vector3 tr = {p0.x + right.x, p0.y + right.y, p0.z + right.z};
            Vector3 bl = {p1.x - right.x, p1.y - right.y, p1.z - right.z};
            Vector3 br = {p1.x + right.x, p1.y + right.y, p1.z + right.z};

            Vector2 dummyUV = {0, 0};

            // 表面 (時計回り)
            mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, normal};
            mappedVertexData[vIndex++] = {{tr.x, tr.y, tr.z, 1.0f}, dummyUV, normal};
            mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, normal};
            mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, normal};
            mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, normal};
            mappedVertexData[vIndex++] = {{bl.x, bl.y, bl.z, 1.0f}, dummyUV, normal};

            // 裏面 (反時計回り - どの角度からでも見えるように必須)
            mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
            mappedVertexData[vIndex++] = {{bl.x, bl.y, bl.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
            mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
            mappedVertexData[vIndex++] = {{tl.x, tl.y, tl.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
            mappedVertexData[vIndex++] = {{br.x, br.y, br.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
            mappedVertexData[vIndex++] = {{tr.x, tr.y, tr.z, 1.0f}, dummyUV, {-normal.x, -normal.y, -normal.z}};
        };

    // 面の生成関数（原点 0,0,0 に作る）
    auto BuildFlatWeb = [&](const Vector3& rightVec, const Vector3& upVec) {
        Vector3 center = {0, 0, 0};
        float radius = 1.0f; // 基準サイズは1.0
        Vector3 normal = {
            rightVec.y * upVec.z - rightVec.z * upVec.y,
            rightVec.z * upVec.x - rightVec.x * upVec.z,
            rightVec.x * upVec.y - rightVec.y * upVec.x
        };

        std::vector<Vector3> spokeEnds(numSpokes);
        for (int i = 0; i < numSpokes; ++i) {
            float angle = (360.0f / numSpokes) * i * (PI / 180.0f);
            float c = std::cos(angle) * radius;
            float s = std::sin(angle) * radius;
            Vector3 endPos = {center.x + (rightVec.x * c) + (upVec.x * s), center.y + (rightVec.y * c) + (upVec.y * s), center.z + (rightVec.z * c) + (upVec.z * s)};
            PushLineQuad(center, endPos, threadThickness, normal);
            spokeEnds[i] = endPos;
        }

        for (int r = 1; r <= numRings; ++r) {
            float t = static_cast<float>(r) / numRings;
            for (int i = 0; i < numSpokes; ++i) {
                int next_i = (i + 1) % numSpokes;
                Vector3 p0 = {center.x + spokeEnds[i].x * t, center.y + spokeEnds[i].y * t, center.z + spokeEnds[i].z * t};
                Vector3 p1 = {center.x + spokeEnds[next_i].x * t, center.y + spokeEnds[next_i].y * t, center.z + spokeEnds[next_i].z * t};
                PushLineQuad(p0, p1, threadThickness * 0.8f, normal);
            }
        }
        };

    // X字に2枚作る
    BuildFlatWeb({0.707f, 0.0f, 0.707f}, {0.0f, 1.0f, 0.0f});
    BuildFlatWeb({0.707f, 0.0f, -0.707f}, {0.0f, 1.0f, 0.0f});

    vertexBuffer_->Unmap(0, nullptr);
    verticesPerWeb_ = vIndex; // 実際の頂点数を記録

    // ==========================================
    // 2. インスタンス用・定数バッファの初期化
    // ==========================================
    // 蜘蛛の巣100個分の「位置データ」を入れるバッファ
    instanceBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(InstanceData) * maxWebs_);
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_));

    viewProjMatrixResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    viewProjMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedViewProjMatrix_));
    //*mappedViewProjMatrix_ = Makeidetity4x4();

    materialResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));
    MaterialData* matData;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&matData));
    matData->color = {1.0f, 1.0f, 1.0f, 1.0f}; // 白色
    matData->enableLighting = 0;
    materialResource_->Unmap(0, nullptr);
}

void SpiderWebRenderer::Update(const std::vector<SpiderWebData>& webs, Camera* camera) {
    currentWebCount_ = static_cast<int>(webs.size());
    if (currentWebCount_ > maxWebs_) currentWebCount_ = maxWebs_;

    // 毎フレームやるのは行列計算だけ！超軽量！
    for (int i = 0; i < currentWebCount_; ++i) {
        // スケール行列
        Matrix4x4 scaleMat = MakeScaleMatrix({webs[i].scale, webs[i].scale, webs[i].scale});
        // 平行移動行列
        Matrix4x4 transMat = MakeTranslateMatrix(webs[i].position);

        // World行列 = Scale * Translate (回転が必要ならここで追加)
        mappedInstanceData_[i].World = Multiply(scaleMat, transMat);
    }

    if (camera) {
    // ★ 構造体の WVP 部分にカメラ行列を入れる。
    // 他の World 等は SpiderWeb では使わないが、念のため単位行列で埋めて安全を確保する。
    mappedViewProjMatrix_->WVP = camera->GetViewProtectionMatrix();
    mappedViewProjMatrix_->World = Makeidetity4x4();
    mappedViewProjMatrix_->WorldInverseTranspose = Makeidetity4x4();
}
}

void SpiderWebRenderer::Draw() {
    if (currentWebCount_ == 0) return;

    auto cmdList = DXCommon::GetInstance()->GetCommandList();

    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, viewProjMatrixResource_->GetGPUVirtualAddress());

    // ★ インスタンスバッファをセット (※SRVとしてセットする前提です)
    // cmdList->SetGraphicsRootDescriptorTable(2, ...); 

    cmdList->IASetVertexBuffers(0, 1, &vbView_);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ★ 魔法の関数！ [1個の巣の頂点数] を [巣の個数分] 一気に描画しろ！
    cmdList->DrawInstanced(verticesPerWeb_, currentWebCount_, 0, 0);
}