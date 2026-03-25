#define NOMINMAX
#include "CollisionMask.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "StringUtility.h"
#include "Vector4Function.h"
#include "ImGuiManager.h"
#include "Logger.h"

std::unique_ptr<CollisionMask, CollisionMask::Deleter> CollisionMask::instance_ = nullptr;

CollisionMask* CollisionMask::GetInstance() 
{
    if (instance_ == nullptr)
    {
        instance_.reset(new CollisionMask());
    }
    return instance_.get();
};

void CollisionMask::Finalize()
{
    instance_.reset();
}

void CollisionMask::Initialize() 
{
   
    maskDatas_.resize(4);

    for (size_t i = 0; i < 4; i++)
    {

        std::unique_ptr<MaskData> maskData{};

        std::unique_ptr<Object3d>object = std::make_unique<Object3d>();
        object->Initialize();
        object->SetRotate(Vector3{ -90.0f / 180.0f * PI, 0.0f, 0.0f });
        object->SetTranslate(translate_);
        
        maskDatas_[i] = std::make_unique<MaskData>();
        maskDatas_[i]->object = std::move(object);
        

    }

    LoadFromFile("resources/Mask/Mask.png", maskDatas_[0]->textureData);
    maskDatas_[0]->name = "mapMaskData";
    ModelManager::GetInstance()->CreatePlaneFromTex(maskDatas_[0]->name, "resources/Mask/Mask.png");
    LoadFromFile("resources/Mask/Mask(1).png", maskDatas_[1]->textureData);
    maskDatas_[1]->name = "mapMaskData1";
    ModelManager::GetInstance()->CreatePlaneFromTex(maskDatas_[1]->name, "resources/Mask/Mask(1).png");
    LoadFromFile("resources/Mask/Mask(2).png", maskDatas_[2]->textureData);
    maskDatas_[2]->name = "mapMaskData2";
    ModelManager::GetInstance()->CreatePlaneFromTex(maskDatas_[2]->name, "resources/Mask/Mask(2).png");
    LoadFromFile("resources/Mask/Mask(3).png", maskDatas_[3]->textureData);
    maskDatas_[3]->name = "mapMaskData3";
    ModelManager::GetInstance()->CreatePlaneFromTex(maskDatas_[3]->name, "resources/Mask/Mask(3).png");

    for (size_t i = 0; i < 4; i++)
    {
        GenerateSDF(maskDatas_[i].get());

        maskDatas_[i]->object->SetModel(maskDatas_[i]->name);

        auto model = ModelManager::GetInstance()->findModel(maskDatas_[i]->name);

        maskDatas_[i]->max_ = model->GetModelData().vertices[1].position;
        maskDatas_[i]->min_ = model->GetModelData().vertices[2].position;
    }
    
    currentMaskMap_ = CollisionMask::MaskMap::Map1;

    maskMapRequest_ = CollisionMask::MaskMap::Unknown;

    PsoConfig config{};
    config.vsPath = L"resources/shaders/MaskMap/Mask.VS.hlsl";
    config.psPath = L"resources/shaders/MaskMap/Mask.PS.hlsl";


    config.rootSignatureGenerator = []() {
        std::vector<D3D12_ROOT_PARAMETER> rootParameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler = PSOManager::GetInstance()->StaticSamplers();

        staticSamplers.push_back(sampler);
        D3D12_DESCRIPTOR_RANGE descRangeTexture[1]{};
        descRangeTexture[0].BaseShaderRegister = 0; // t0
        descRangeTexture[0].NumDescriptors = 1;
        descRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



        rootParameters.resize(8);



        // Enum定義 (可読性のため)
        enum {
            kMaterial, kTransform, kTexture, DirLight, PointLight, SpotLight, Count, kCamera
        };

        // 0. Material (CBV b0, Pixel)
        rootParameters[kMaterial].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[kMaterial].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[kMaterial].Descriptor.ShaderRegister = 0;

        // 1. Transform (CBV b0, Vertex)
        rootParameters[kTransform].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[kTransform].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[kTransform].Descriptor.ShaderRegister = 0;

        // 2. Texture (Table t0, Pixel)
        rootParameters[kTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[kTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[kTexture].DescriptorTable.pDescriptorRanges = descRangeTexture;
        rootParameters[kTexture].DescriptorTable.NumDescriptorRanges = 1;

        // ★変更: 3. DirectionalLight (SRV t1)
        rootParameters[DirLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[DirLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[DirLight].Descriptor.ShaderRegister = 1; // t1

        // ★追加: 4. PointLight (SRV t2)
        rootParameters[PointLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[PointLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[PointLight].Descriptor.ShaderRegister = 2; // t2

        // ★追加: 5. SpotLight (SRV t3)
        rootParameters[SpotLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[SpotLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[SpotLight].Descriptor.ShaderRegister = 3; // t3

        // ★追加: 6. LightCounts (CBV b3)
        rootParameters[Count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[Count].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[Count].Descriptor.ShaderRegister = 3; // b3 (b0,b1,b2は使用済みと仮定、あるいは空いている番号)
        //カメラ
        rootParameters[kCamera].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファビュー
        rootParameters[kCamera].Descriptor.ShaderRegister = 2; // レジスタ番号 2 (b2)
        rootParameters[kCamera].Descriptor.RegisterSpace = 0;
        rootParameters[kCamera].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーのみ見える

        // シリアライズ
        D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
        descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        descriptionRootSignature.pParameters = rootParameters.data();
        descriptionRootSignature.NumParameters = (UINT)rootParameters.size();
        descriptionRootSignature.pStaticSamplers = staticSamplers.data();
        descriptionRootSignature.NumStaticSamplers = (UINT)staticSamplers.size();


        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
        hr = DXCommon::GetInstance()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        assert(SUCCEEDED(hr));



        return rootSignature;
        };
    config.inputLayoutGenerator = []() {
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        };
    // 深度設定
    config.depthEnable = true;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    // PSOManagerに名前を付けて登録
    PSOManager::GetInstance()->RegisterPsoGenerator("MaskMap", config);

    for (size_t i = 0; i < 4; i++)
    {
        maskDatas_[i].get()->object->SetPsoName("MaskMap");
    }
}

void CollisionMask::Update() 
{
    if(maskMapRequest_ != MaskMap::Unknown)
    {
        currentMaskMap_ = maskMapRequest_;
        maskMapRequest_ = MaskMap::Unknown;
    }

#ifdef _DEBUG

    ImGui::Begin("MaskMap Setting");

    int maskMapIndex = static_cast<int>(currentMaskMap_);
    const char* items[] = { "Map1", "Map2", "Map3","Map4"};
    if (ImGui::Combo("Mask Map", &maskMapIndex, items, IM_ARRAYSIZE(items)))
    {
        SetMaskMapRequest(static_cast<MaskMap>(maskMapIndex));
    }

    ImGui::End();

#endif // _DEBUG
    

    maskDatas_[static_cast<int>(currentMaskMap_)]->object->Update();
}

void CollisionMask::Draw() 
{
    maskDatas_[static_cast<int>(currentMaskMap_)]->object->Draw();
}

bool CollisionMask::LoadFromFile(const std::string& filePath, TextureData& textureData)
{
    DirectX::ScratchImage scrachImage{};
    std::wstring filePathW = StringUtility::ConvertString(filePath);

    // 1. ロード
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, scrachImage);
    if (FAILED(hr)) return false;

    TextureManager::GetInstance()->LoadTexture(filePath);

    // WICで強制デコードして直接convertedに
    DirectX::ScratchImage converted{};
    hr = DirectX::LoadFromWICFile(filePathW.c_str(),
        DirectX::WIC_FLAGS_FORCE_RGB,
        nullptr,
        converted);
    assert(SUCCEEDED(hr));

    // DXGI_FORMAT_R8G8B8A8_UNORM に変換
    const DirectX::Image* img = converted.GetImage(0, 0, 0);
    if (img && img->format != DXGI_FORMAT_R8G8B8A8_UNORM) {
        DirectX::ScratchImage tmp;
        hr = DirectX::Convert(*img, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, 0, tmp);
        assert(SUCCEEDED(hr));
        converted = std::move(tmp);
    }

    const DirectX::Image* image = converted.GetImage(0, 0, 0);
    textureData.widthX = static_cast<int>(image->width);
    textureData.widthZ = static_cast<int>(image->height);
    textureData.data.assign(textureData.widthX * textureData.widthZ, 0);

    for (size_t y = 0; y < image->height; ++y) {
        for (size_t x = 0; x < image->width; ++x) {
            // ピクセル位置のポインタを計算 (RGBA8を想定)
            uint8_t* pPixel = image->pixels + (y * image->rowPitch) + (x * 4);

            // R, G, B の平均値などで判定 (あるいはAlpha値)
            uint8_t brightness = (pPixel[0] + pPixel[1] + pPixel[2]) / 3;
            textureData.data[y * textureData.widthX + x] = brightness;
        }
    }

    return false;
}

void CollisionMask::GenerateSDF(MaskData* mask)
{
    int w = mask->textureData.widthX;
    int h = mask->textureData.widthZ;
    mask->sdfData.resize(w * h); // float型の配列を用意

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // 全ピクセルを走査して最短距離を計算
            // (本来は 8-point SSEDT 等の高速アルゴリズムを使いますが、まずは二重ループで)
            float minDist = FindNearestWallDist(x, y, mask);
            mask->sdfData[y * w + x] = minDist;
        }
    }
}

float CollisionMask::FindNearestWallDist(int startX, int startZ, MaskData* mask)
{
    int w = mask->textureData.widthX;
    int h = mask->textureData.widthZ;

    // 探索する最大半径（これより遠い壁は無視していいことにする）
    // マップ全体のサイズに合わせて調整。例：32ピクセル分など
    const int maxSearchRange = 64;
    float minSqDist = static_cast<float>(maxSearchRange * maxSearchRange);
    bool found = false;

    // 現在のピクセルがすでに壁（黒）なら、距離は 0 (または負の深さ)
    if (mask->textureData.data[startZ * w + startX] < 128) {
        return 0.0f; // 簡易版なので壁の中は0
    }

    // 周囲を矩形状に探索
    for (int dz = -maxSearchRange; dz <= maxSearchRange; ++dz) {
        for (int dx = -maxSearchRange; dx <= maxSearchRange; ++dx) {
            int targetX = startX + dx;
            int targetZ = startZ + dz;

            // 範囲外チェック
            if (targetX < 0 || targetX >= w || targetZ < 0 || targetZ >= h) continue;

            // そのピクセルが壁（黒）かどうか
            if (mask->textureData.data[targetZ * w + targetX] < 128) {
                float sqDist = static_cast<float>(dx * dx + dz * dz);
                if (sqDist < minSqDist) {
                    minSqDist = sqDist;
                    found = true;
                }
            }
        }
    }

    return found ? std::sqrt(minSqDist) : static_cast<float>(maxSearchRange);
}

float CollisionMask::GetSDFValue(float worldX, float worldZ)
{
    auto& maskData = maskDatas_[static_cast<int>(currentMaskMap_)];

    // 1. ワールド座標を画像上の浮動小数点の座標 (u, v) に変換
    float u = (worldX - maskData->min_.x) / (maskData->max_.x - maskData->min_.x) 
        * (maskData->textureData.widthX - 1);
    float v = (worldZ - maskData->min_.y) / (maskData->max_.y - maskData->min_.y) 
        * (maskData->textureData.widthZ - 1); 

    // 2. 周辺4ピクセルの座標を特定（確実に範囲内に収める）
    int x0 = std::clamp(static_cast<int>(u), 0, maskData->textureData.widthX - 1);
    int y0 = std::clamp(static_cast<int>(v), 0, maskData->textureData.widthZ - 1);
    int x1 = std::clamp(x0 + 1, 0, maskData->textureData.widthX - 1);
    int y1 = std::clamp(y0 + 1, 0, maskData->textureData.widthZ - 1);

    // 3. ピクセル内の余り（0.0〜1.0）
    float fx = u - x0;
    float fy = v - y0;

    // 4. 4点のSDF値を線形補間
    float d00 = maskData->sdfData[y0 * maskData->textureData.widthX + x0];
    float d10 = maskData->sdfData[y0 * maskData->textureData.widthX + x1];
    float d01 = maskData->sdfData[y1 * maskData->textureData.widthX + x0];
    float d11 = maskData->sdfData[y1 * maskData->textureData.widthX + x1];

    float d0 = d00 * (1 - fx) + d10 * fx;
    float d1 = d01 * (1 - fx) + d11 * fx;

    float pixelDist = d0 * (1 - fy) + d1 * fy;

    // --- ここが重要！ピクセル距離をワールド距離(メートル)に変換 ---
    // 1ピクセルあたりのワールド座標での長さを求める
    float worldRangeX = std::abs(maskData->max_.x - maskData->min_.x);
    float worldPerPixel = worldRangeX / (maskData->textureData.widthX - 1);

    return pixelDist * worldPerPixel;
}

Vector2 CollisionMask::GetSDFNormal(float worldX, float worldZ)
{
    const float delta = 1.0f;

    // 動いた時の距離の変化を見る
    float dx = GetSDFValue(worldX + delta, worldZ) - GetSDFValue(worldX - delta, worldZ);
    float dz = GetSDFValue(worldX, worldZ + delta) - GetSDFValue(worldX, worldZ - delta);

    float length = std::sqrtf(dx * dx + dz * dz);
    if(length < 1e-5f) {
        // 勾配がほとんどない（距離場が平坦）場合は、適当な法線を返す
        return Vector2(0.0f, 0.0f);
    }

    return Vector2{ dx / length, dz / length };
}



bool CollisionMask::IsWall(float x, float z) const
{


    Vector4 min_ = maskDatas_[static_cast<int>(currentMaskMap_)]->min_;
    Vector4 max_ = maskDatas_[static_cast<int>(currentMaskMap_)]->max_;
    int widthX = maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.widthX;
    int widthZ = maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.widthZ;

    float u = ((x - min_.x) / (max_.x - min_.x)) * widthX;
    float v = ((z - min_.y) / (max_.y - min_.y)) * widthZ;

    int ix = static_cast<int>(u);
    int iz = static_cast<int>(v);

    // 画面外は壁として扱う
    if (ix < 0 || ix >= widthX || iz < 0 || iz >= widthZ) return true;

    // インデックス計算（1ピクセル1バイトの場合）
    return maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.data[iz * widthX + ix] < 128; // 閾値で判定
}

bool CollisionMask::IsCollisionWall(const float& x, const float& z, const float& width)
{
    Vector4 min_ = maskDatas_[static_cast<int>(currentMaskMap_)]->min_;
    Vector4 max_ = maskDatas_[static_cast<int>(currentMaskMap_)]->max_;
    int widthX = maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.widthX;
    int widthZ = maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.widthZ;

    float left  = x - (width / 2.0f);
    float right = x + (width / 2.0f);
    float front = z - (width / 2.0f);
    float back = z + (width / 2.0f);
  
    float v1 = (front - min_.y) / (max_.y - min_.y) * widthZ;
    float v2 = (back - min_.y) / (max_.y - min_.y) * widthZ;
    int minIZ = std::clamp(static_cast<int>(std::min(v1, v2)), 0, widthZ - 1);
    int maxIZ = std::clamp(static_cast<int>(std::max(v1, v2)), 0, widthZ - 1);

    float u1 = (left - min_.x) / (max_.x - min_.x) * widthX;
    float u2 = (right - min_.x) / (max_.x - min_.x) * widthX;
    int minIX = std::clamp(static_cast<int>(std::min(u1, u2)), 0, widthX - 1);
    int maxIX = std::clamp(static_cast<int>(std::max(u1, u2)), 0, widthX - 1);

    // 範囲内のピクセルを走査
    for (int iz = minIZ; iz <= maxIZ; ++iz) {
        for (int ix = minIX; ix <= maxIX; ++ix) 
        {
            // 黒ピクセルがあれば衝突
            if (maskDatas_[static_cast<int>(currentMaskMap_)]->textureData.data[iz * widthX + ix] < 128)
            {
            #ifdef _DEBUG

                ImGui::Begin("Debug");
                ImGui::Text("Hit at Pixel: X=%d, Z=%d", ix, iz);
                ImGui::End();

                return true;

            #endif // _DEBUG

            }
        }
    }

    return false;
}

CollisionMask::RayResult CollisionMask::CastRayThroughWall(Vector3 start, Vector3 direction, float maxDist)
{
    RayResult result;
    float traveled = 0.0f;

    // 進行方向をXZ平面（水平）に限定して正規化
    float len = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (len < 0.001f) return result;
    Vector2 dirH = { direction.x / len, direction.z / len };

    // ステップ1: 壁の入り口を探す
    while (traveled < maxDist) {
        Vector3 currentPos; 
        currentPos.x = start.x + direction.x * traveled;
        currentPos.z = start.z + direction.z * traveled;
        float dist = GetSDFValue(currentPos.x, currentPos.z);

        // 距離がほぼ0（壁の表面）に到達
        if (dist <= 0.1f) {
            result.hit = true;
            result.hitPos = {currentPos.x, currentPos.z};
            break;
        }
        // 安全な距離分だけ進む
        traveled += std::max(dist, 0.1f);
    }

    if (!result.hit) return result;

    // ステップ2: そのまま突き進んで「出口」を探す
    // 壁の中では SDF 値が 0 になるように FindNearestWallDist で作ったので、
    // ここでは一定間隔（1ピクセル分など）で少しずつ進みます
    float exitTraveled = traveled + 0.5f;
    bool foundExit = false; // ★ 出口を見つけたかどうかのフラグを追加

    while (exitTraveled < maxDist) {
        Vector3 currentPos;
        currentPos.x = start.x + direction.x * exitTraveled;
        currentPos.z = start.z + direction.z * exitTraveled;
        float dist = GetSDFValue(currentPos.x, currentPos.z);

        // 距離が再びプラス（空白）になったらそこが出口
        if (dist > 0.1f) {
            result.exitPos = {currentPos.x, currentPos.z};
            foundExit = true; // ★ 出口を発見！
            break;
        }
        exitTraveled += 0.5f; // 壁の中は慎重に進む
    }

    // ★ 追加：出口が見つからずに maxDist まで到達してしまった場合
    if (!foundExit) {
        // 「貫通できる壁ではなかった（虚空に繋がっていた）」として、ヒット判定自体を取り消す
        result.hit = false;
    }

    return result;
}
