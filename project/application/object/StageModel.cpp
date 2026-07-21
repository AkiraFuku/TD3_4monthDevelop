#include "StageModel.h"
#include "ModelManager.h"
#include "CollisionMask.h"

void StageModel::Initialize()
{

}

void StageModel::Finalize()
{
    Release();
}

void StageModel::Update()
{
    if (object3dFloor_)
    {
        object3dFloor_->Update();
    }
    if (object3dWall_)
    {
        object3dWall_->Update();
    }
}

void StageModel::Draw()
{
    if (object3dFloor_)
    {
        object3dFloor_->Draw();
    }
    if (object3dWall_)
    {
        object3dWall_->Draw();
    }
}

void StageModel::Create(float wallHeight)
{
    // 既存モデルの解放
    Release();

    CollisionMask* collisionMask = CollisionMask::GetInstance();
    auto mask = collisionMask->GetMaskData(collisionMask->GetCurrentStageID());

    int w = mask->textureData.widthX;
    int h = mask->textureData.widthZ;
    auto& data = mask->textureData.data;

    std::vector<Model::VertexData> verticesFloor;
    std::vector<Model::VertexData> verticesWall;

    // --- 1. 蓋 (道ピクセルを埋める) ---
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            if (data[x][z] >= 128) { // 白＝道
                int startX = x;
                while (x + 1 < w && data[x + 1][z] >= 128) x++;
                AddCeilingPanel((float)startX, (float)z, (float)x + 1, (float)z + 1, wallHeight, verticesFloor);
            }
        }
    }

    // --- 2. 壁 (道と奈落の境界に壁を作る) ---

    // 北向きの壁 (上が奈落)
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            if (data[x][z] >= 128 && (z == 0 || data[x][(z - 1)] < 128)) {
                int startX = x;
                while (x + 1 < w && data[(x + 1)][z] >= 128 && (z == 0 || data[(x + 1)][(z - 1)] < 128)) x++;
                AddWallPanel((float)startX, (float)z, (float)x + 1, (float)z, wallHeight, verticesWall);

            }
        }
    }

    // 南向きの壁 (下が奈落)
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            if (data[x][z] >= 128 && (z == h - 1 || data[x][(z + 1)] < 128)) {
                int startX = x;
                while (x + 1 < w && data[(x + 1)][z] >= 128 && (z == h - 1 || data[(x + 1)][(z + 1)] < 128)) x++;
                AddWallPanel((float)startX, (float)z + 1, (float)x + 1, (float)z + 1, wallHeight, verticesWall, true);

            }
        }
    }

    // 東向きの壁 (右が奈落)
    for (int x = 0; x < w; ++x) {
        for (int z = 0; z < h; ++z) {
            if (data[x][z] >= 128 && (x == w - 1 || data[(x + 1)][z] < 128)) {
                int startZ = z;
                while (z + 1 < h && data[x][(z + 1)] >= 128 && (x == w - 1 || data[(x + 1)][(z + 1)] < 128)) z++;
                AddWallPanel((float)x + 1, (float)startZ, (float)x + 1, (float)z + 1, wallHeight, verticesWall);

            }
        }
    }

    // 西向きの壁 (左が奈落)
    for (int x = 0; x < w; ++x) {
        for (int z = 0; z < h; ++z) {
            if (data[x][z] >= 128 && (x == 0 || data[(x - 1)][z] < 128)) {
                int startZ = z;
                while (z + 1 < h && data[x][(z + 1)] >= 128 && (x == 0 || data[(x - 1)][(z + 1)] < 128)) z++;
                AddWallPanel((float)x, (float)startZ, (float)x, (float)z + 1, wallHeight, verticesWall, true);
            }
        }
    }


    //// 現在のピクセルが壁（黒：<128）なら処理
    //if (data[z * w + x] < 128) {

    //    // 右隣をチェック：右が範囲外、または道なら、そこに「右側の壁面」を作る
    //    if (x + 1 >= w || data[z * w + (x + 1)] >= 128) {
    //        AddWallPanel((float)x + 1, (float)z, (float)x + 1, (float)z + 1, wallHeight, vertices);
    //    }
    //    // 左隣をチェック
    //    if (x - 1 < 0 || data[z * w + (x - 1)] >= 128) {
    //        AddWallPanel((float)x, (float)z + 1, (float)x, (float)z, wallHeight, vertices);
    //    }
    //    // 下隣（奥）をチェック
    //    if (z + 1 >= h || data[(z + 1) * w + x] >= 128) {
    //        AddWallPanel((float)x + 1, (float)z + 1, (float)x, (float)z + 1, wallHeight, vertices);
    //    }
    //    // 上隣（手前）をチェック
    //    if (z - 1 < 0 || data[(z - 1) * w + x] >= 128) {
    //        AddWallPanel((float)x, (float)z, (float)x + 1, (float)z, wallHeight, vertices);
    //    }
    //} else
    //{
    //    // 壁ピクセルの真上に水平なパネルを置く
    //    AddCeilingPanel((float)x, (float)z, wallHeight, vertices);
    //}


    if (verticesWall.empty()) return;

    // ModelManagerに動的メッシュとして登録
    // ※お使いのModelManagerに「頂点配列からモデルを作る」機能がある想定です
    modelNameFloor_ = "StageModelFloor_" + std::to_string(collisionMask->GetCurrentStageID());
    ModelManager::GetInstance()->CreateDynamicModel(modelNameFloor_, verticesFloor, textureFilePathFloor_);

    object3dFloor_ = std::make_unique<Object3d>();
    object3dFloor_->Initialize();
    object3dFloor_->SetModel(modelNameFloor_);
    object3dFloor_->SetTranslate({ 0.0f, -1.0f, 0.0f });
    object3dFloor_->SetPsoName("Object3d");

    modelNameWall_ = "StageModelWall_" + std::to_string(collisionMask->GetCurrentStageID());
    ModelManager::GetInstance()->CreateDynamicModel(modelNameWall_, verticesWall, textureFilePathWall_);

    object3dWall_ = std::make_unique<Object3d>();
    object3dWall_->Initialize();
    object3dWall_->SetModel(modelNameWall_);
    object3dWall_->SetTranslate({ 0.0f, -1.0f, 0.0f });
    object3dWall_->SetPsoName("Object3d");
}

void StageModel::Release()
{
    if (object3dFloor_)
    {
        ModelManager::GetInstance()->RemoveModel(modelNameFloor_);
        object3dFloor_.reset();
    }

    if (object3dWall_)
    {
        ModelManager::GetInstance()->RemoveModel(modelNameWall_);
        object3dWall_.reset();
    }
}

void StageModel::AddWallPanel(float x1, float z1, float x2, float z2, float height, 
   std::vector<Model::VertexData>&vertices, bool flipNormal)
{
    CollisionMask* collisionMask = CollisionMask::GetInstance();
    auto mask = collisionMask->GetMaskData(collisionMask->GetCurrentStageID());
    float texW = (float)mask->textureData.widthX;
    float texZ = (float)mask->textureData.widthZ;

    auto ScreenToWorld = [&](float px, float pz){
        float wx = mask->min_.x + (px / texW) * (mask->max_.x - mask->min_.x);
        float wz = mask->min_.y + (pz / texZ) * (mask->max_.y - mask->min_.y);
        return Vector3{ wx, 0.0f, wz };
        };

    Vector3 p1 = ScreenToWorld(x1, z1);
    Vector3 p2 = ScreenToWorld(x2, z2);

    Vector3 edge = { p2.x - p1.x, 0.0f, p2.z - p1.z };
    Vector3 normal = flipNormal
        ? Vector3{ -edge.z, 0.0f,  edge.x }
    : Vector3{ edge.z, 0.0f, -edge.x };

    Model::VertexData lt = { {p1.x,    0.0f, p1.z, 1.0f}, {0.0f, 0.0f}, normal };
    Model::VertexData rt = { {p2.x,    0.0f, p2.z, 1.0f}, {1.0f, 0.0f}, normal };
    Model::VertexData lb = { {p1.x, -height, p1.z, 1.0f}, {0.0f, 1.0f}, normal };
    Model::VertexData rb = { {p2.x, -height, p2.z, 1.0f}, {1.0f, 1.0f}, normal };

    if (!flipNormal) {
        // 北・東: 現状カリング正常なので巻き順そのまま、UVだけ直す
        vertices.push_back(lt);
        vertices.push_back(lb);
        vertices.push_back(rt);
        vertices.push_back(rt);
        vertices.push_back(lb);
        vertices.push_back(rb);
    } else {
        // 南・西: 巻き順を反転してカリングを修正
        vertices.push_back(rt);
        vertices.push_back(lb);
        vertices.push_back(lt);
        vertices.push_back(rb);
        vertices.push_back(lb);
        vertices.push_back(rt);
    }
}

void StageModel::AddCeilingPanel(float x1, float z1, float x2, float z2, float height,
    std::vector<Model::VertexData>& vertices) {

    CollisionMask* collisionMask = CollisionMask::GetInstance();
    auto mask = collisionMask->GetMaskData(collisionMask->GetCurrentStageID());

    // ステージ全体のピクセルサイズ
    float texW = (float)mask->textureData.widthX;
    float texZ = (float)mask->textureData.widthZ;

    auto ScreenToWorld = [&](float px, float pz){
        float wx = mask->min_.x + (px / texW) * (mask->max_.x - mask->min_.x);
        float wz = mask->min_.y + (pz / texZ) * (mask->max_.y - mask->min_.y);
        return Vector3{ wx, 0.0f, wz }; // 蓋の高さ(y=0)
    };

    Vector3 pLT = ScreenToWorld(x1, z1);
    Vector3 pRT = ScreenToWorld(x2, z1);
    Vector3 pLB = ScreenToWorld(x1, z2);
    Vector3 pRB = ScreenToWorld(x2, z2);

    // --- UV計算: ステージ全体を (0,0)〜(1,1) とする ---
    float u1 = x1 / texW;
    float u2 = x2 / texW;
    float v1 = z1 / texZ;
    float v2 = z2 / texZ;

    Vector3 normal = { 0.0f, 1.0f, 0.0f };

    // 頂点データ
    Model::VertexData vLT = { {pLT.x, pLT.y, pLT.z, 1.0f}, {u1, v1}, normal };
    Model::VertexData vRT = { {pRT.x, pRT.y, pRT.z, 1.0f}, {u2, v1}, normal };
    Model::VertexData vLB = { {pLB.x, pLB.y, pLB.z, 1.0f}, {u1, v2}, normal };
    Model::VertexData vRB = { {pRB.x, pRB.y, pRB.z, 1.0f}, {u2, v2}, normal };

    // 時計回りで追加（上が見えるように）
    vertices.push_back(vLT);
    vertices.push_back(vRT);
    vertices.push_back(vRB);

    vertices.push_back(vLT);
    vertices.push_back(vRB);
    vertices.push_back(vLB);
}