#include "CollisionMask.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "StringUtility.h"
#include "Vector4.h"
#include "ImGuiManager.h"

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
   
    maskDatas_.resize(2);

    for (size_t i = 0; i < 2; i++)
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

    LoadFromFile("resources/Mask/Mask(2).png", maskDatas_[1]->textureData);
    maskDatas_[1]->name = "mapMaskData2";
    ModelManager::GetInstance()->CreatePlaneFromTex(maskDatas_[1]->name, "resources/Mask/Mask(2).png");

    for (size_t i = 0; i < 2; i++)
    {
        maskDatas_[i]->object->SetModel(maskDatas_[i]->name);

        auto model = ModelManager::GetInstance()->findModel(maskDatas_[i]->name);

        maskDatas_[i]->max_ = model->GetModelData().vertices[1].position;
        maskDatas_[i]->min_ = model->GetModelData().vertices[2].position;
    }
    
    currentMaskMap_ = CollisionMask::MaskMap::Map1;

    maskMapRequest_ = CollisionMask::MaskMap::Unknown;
}

void CollisionMask::Update() 
{
    if(maskMapRequest_ != MaskMap::Unknown)
    {
        currentMaskMap_ = maskMapRequest_;
        maskMapRequest_ = MaskMap::Unknown;
    }

    ImGui::Begin("MaskMap Setting");

    int maskMapIndex = static_cast<int>(currentMaskMap_);
    const char* items[] = {"Map1", "Map2"};
    if (ImGui::Combo("Mask Map", &maskMapIndex, items, IM_ARRAYSIZE(items)))
    {
        SetMaskMapRequest(static_cast<MaskMap>(maskMapIndex));
    }

    ImGui::End();

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

    // 2. 判定しやすいように強制変換 (RGBA 8bit)
    DirectX::ScratchImage converted{};
    hr = DirectX::Convert(
        *scrachImage.GetImage(0, 0, 0),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DirectX::TEX_FILTER_DEFAULT, 0,
        converted
    );

    if (FAILED(hr)) return false;

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