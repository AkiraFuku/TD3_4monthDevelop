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
    if (object3d_)
    {
        object3d_->Update();
    }
}

void StageModel::Draw()
{
    if (object3d_)
    {
        object3d_->Draw();
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

    std::vector<Model::VertexData> vertices;

    // 全ピクセルを走査して「道(白)」と「壁(黒)」の境界を探す
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            // 現在のピクセルが壁（黒：<128）なら処理
            if (data[z * w + x] < 128) {

                // 右隣をチェック：右が範囲外、または道なら、そこに「右側の壁面」を作る
                if (x + 1 >= w || data[z * w + (x + 1)] >= 128) {
                    AddWallPanel((float)x + 1, (float)z, (float)x + 1, (float)z + 1, wallHeight, vertices);
                }
                // 左隣をチェック
                if (x - 1 < 0 || data[z * w + (x - 1)] >= 128) {
                    AddWallPanel((float)x, (float)z + 1, (float)x, (float)z, wallHeight, vertices);
                }
                // 下隣（奥）をチェック
                if (z + 1 >= h || data[(z + 1) * w + x] >= 128) {
                    AddWallPanel((float)x + 1, (float)z + 1, (float)x, (float)z + 1, wallHeight, vertices);
                }
                // 上隣（手前）をチェック
                if (z - 1 < 0 || data[(z - 1) * w + x] >= 128) {
                    AddWallPanel((float)x, (float)z, (float)x + 1, (float)z, wallHeight, vertices);
                }
            }
            else
            {
                // 壁ピクセルの真上に水平なパネルを置く
                AddCeilingPanel((float)x, (float)z, wallHeight, vertices);
            }
        }
    }

    if (vertices.empty()) return;

    // ModelManagerに動的メッシュとして登録
    // ※お使いのModelManagerに「頂点配列からモデルを作る」機能がある想定です
    modelName_ = "GeneratedStage_" + std::to_string(collisionMask->GetCurrentStageID());
    ModelManager::GetInstance()->CreateDynamicModel(modelName_, vertices, textureFilePath_);

    object3d_ = std::make_unique<Object3d>();
    object3d_->Initialize();
    object3d_->SetModel(modelName_);
    object3d_->SetPsoName("MaskMap"); // 既存のPSOを流用
    object3d_->SetTranslate({ 0.0f, -0.5f, 0.0f });

}

void StageModel::Release()
{
    if (object3d_)
    {
        ModelManager::GetInstance()->RemoveModel(modelName_);
        object3d_.reset();
    }
}

void StageModel::AddWallPanel(float x1, float z1, float x2, float z2, float height, 
   std::vector<Model::VertexData>&vertices)
{
    CollisionMask* collisionMask = CollisionMask::GetInstance();
    auto mask = collisionMask->GetMaskData(collisionMask->GetCurrentStageID());

    uint32_t baseIdx = (uint32_t)vertices.size();

    // ピクセル座標からワールド座標への変換関数（CollisionMask内のロジックを流用）
    auto ScreenToWorld = [&](float px, float pz){
        float wx = mask->min_.x + (px / (float)mask->textureData.widthX) * (mask->max_.x - mask->min_.x);
        float wz = mask->min_.y + (pz / (float)mask->textureData.widthZ) * (mask->max_.y - mask->min_.y);
        return Vector3{ wx, 0.0f, wz };
        };

    Vector3 p1 = ScreenToWorld(x1, z1);
    Vector3 p2 = ScreenToWorld(x2, z2);

    // 法線の計算（壁の向き）
    Vector3 edge = { p2.x - p1.x, 0, p2.z - p1.z };
    Vector3 normal = { -edge.z, 0, edge.x }; // 90度回転
    // normal.Normalize(); // 必要に応じて正規化

    //// 4頂点の作成（下2つ、上2つ）
    //vertices.push_back({ {p1.x, 0.0f, p1.z, 1.0f}, {0,1}, normal }); // 下左
    //vertices.push_back({ {p2.x, 0.0f, p2.z, 1.0f}, {1,1}, normal }); // 下右
    //vertices.push_back({ {p1.x, height, p1.z, 1.0f}, {0,0}, normal }); // 上左
    //vertices.push_back({ {p2.x, height, p2.z, 1.0f}, {1,0}, normal }); // 上右

    //// インデックス設定（時計回り）
    /*indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 1);
    indices.push_back(baseIdx + 1); indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);*/


    // 頂点定義
    Model::VertexData lt = { {p1.x, 0.0f,   p1.z, 1.0f}, {0.0f, 1.0f}, normal }; // 左下(LowerLeft)
    Model::VertexData rt = { {p2.x, 0.0f,   p2.z, 1.0f}, {1.0f, 1.0f}, normal }; // 右下(LowerRight)
    Model::VertexData lb = { {p1.x, -height, p1.z, 1.0f}, {0.0f, 0.0f}, normal }; // 左上(UpperLeft)
    Model::VertexData rb = { {p2.x, -height, p2.z, 1.0f}, {1.0f, 0.0f}, normal }; // 右上(UpperRight)

    // --- インデックスを使わず、三角形2枚分（計6頂点）を直接追加 ---

    // 三角形1: 左下 -> 左上 -> 右下
    vertices.push_back(lb);
    vertices.push_back(lt);
    vertices.push_back(rb);

    // 三角形2: 右下 -> 左上 -> 右上
    vertices.push_back(rb);
    vertices.push_back(lt);
    vertices.push_back(rt);
}

void StageModel::AddCeilingPanel(float x, float z, float height,
    std::vector<Model::VertexData>& vertices) 
{

    CollisionMask* collisionMask = CollisionMask::GetInstance();
    auto mask = collisionMask->GetMaskData(collisionMask->GetCurrentStageID());

    auto ScreenToWorld = [&](float px, float pz){
        float wx = mask->min_.x + (px / (float)mask->textureData.widthX) * (mask->max_.x - mask->min_.x);
        float wz = mask->min_.y + (pz / (float)mask->textureData.widthZ) * (mask->max_.y - mask->min_.y);
        return Vector3{ wx, 0.0f, wz }; // 指定の高さで座標計算
        };

    // 1ピクセルに相当する4角の座標を計算
    Vector3 pLT = ScreenToWorld(x, z);           // 左上
    Vector3 pRT = ScreenToWorld(x + 1.0f, z);    // 右上
    Vector3 pLB = ScreenToWorld(x, z + 1.0f);    // 左下
    Vector3 pRB = ScreenToWorld(x + 1.0f, z + 1.0f); // 右下

    Vector3 normal = { 0.0f, 1.0f, 0.0f }; // 法線は上向き

    // 頂点データ作成 (UVの向きも調整)
    Model::VertexData vLT = { {pLT.x, pLT.y, pLT.z, 1.0f}, {0,0}, normal };
    Model::VertexData vRT = { {pRT.x, pRT.y, pRT.z, 1.0f}, {1,0}, normal };
    Model::VertexData vLB = { {pLB.x, pLB.y, pLB.z, 1.0f}, {0,1}, normal };
    Model::VertexData vRB = { {pRB.x, pRB.y, pRB.z, 1.0f}, {1,1}, normal };

    // --- 頂点の結び順を「時計回り」に再構成 ---
    // ※もしこれで出なければ、逆順（vLT, vLB, vRTなど）を試してください

    // 三角形1: 左上 -> 右上 -> 右下
    vertices.push_back(vLT);
    vertices.push_back(vRT);
    vertices.push_back(vRB);

    // 三角形2: 左上 -> 右下 -> 左下
    vertices.push_back(vLT);
    vertices.push_back(vRB);
    vertices.push_back(vLB);
}