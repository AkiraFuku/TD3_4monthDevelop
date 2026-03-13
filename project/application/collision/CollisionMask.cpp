#include "CollisionMask.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "StringUtility.h"
#include "Vector4.h"

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
    LoadFromFile("resources/Mask/Mask.png");
    ModelManager::GetInstance()->CreatePlaneFromTex("mapMaskData", "resources/Mask/Mask.png");

    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("mapMaskData");
    

    auto model = ModelManager::GetInstance()->findModel("mapMaskData");

    max_ = model->GetModelData().vertices[1].position;
    min_ = model->GetModelData().vertices[2].position;
}



void CollisionMask::Update() 
{
    object_->SetRotate(Vector3{ -90.0f / 180.0f * PI, 0.0f, 0.0f });
    object_->Update();
}

void CollisionMask::Draw() 
{
    object_->Draw();
}

bool CollisionMask::LoadFromFile(const std::string& filePath)
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
    widthX = static_cast<int>(image->width);
    widthZ = static_cast<int>(image->height);
    data.assign(widthX * widthZ, 0);

    for (size_t y = 0; y < image->height; ++y) {
        for (size_t x = 0; x < image->width; ++x) {
            // ピクセル位置のポインタを計算 (RGBA8を想定)
            uint8_t* pPixel = image->pixels + (y * image->rowPitch) + (x * 4);

            // R, G, B の平均値などで判定 (あるいはAlpha値)
            uint8_t brightness = (pPixel[0] + pPixel[1] + pPixel[2]) / 3;
            data[y * widthX + x] = brightness;
        }
    }

    return false;
}

bool CollisionMask::IsWall(float x, float z) const
{

    float u = ((x - min_.x) / (max_.x - min_.x)) * widthX;
    float v = ((z - min_.y) / (max_.y - min_.y)) * widthZ;

    int ix = static_cast<int>(u);
    int iz = static_cast<int>(v);

    // 画面外は壁として扱う
    if (ix < 0 || ix >= widthX || iz < 0 || iz >= widthZ) return true;

    // インデックス計算（1ピクセル1バイトの場合）
    return data[iz * widthX + ix] < 128; // 閾値で判定
}