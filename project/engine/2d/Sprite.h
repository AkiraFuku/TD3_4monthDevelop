#pragma once
#include "Vector4.h"
#include "Vector2.h"
#include <wrl.h>
#include <d3d12.h>
#include<string>

#include "PSOManager.h"
class Sprite
{
public:
    struct VertexData {
        Vector4 position; // 4D position vector
        Vector2 texcord; // 2D texture coordinate vector
        Vector3 normal;
    };
    struct Material
    {
        Vector4 color;
        int32_t enableLighting;
        float padding[3]; // 繝代ョ繧｣繝ｳ繧ｰ繧定ｿｽ蜉縺励※繧ｵ繧､繧ｺ繧呈純縺医ｋ
        Matrix4x4 uvTransform; // UV螟画鋤陦悟・

    };
    struct TransformationMatrix
    {
        Matrix4x4 WVP;
        Matrix4x4 World;

    };

    void Initialize( std::string textureFilePath);
    void Update();
    void Draw();

    const Vector2& GetPosition() const {
        return position_;
    }
    void SetPosition(const Vector2& position) {
        position_ = position;
    }

    float GetRotation() const {
        return rotation_;
    }
    void SetRotation(const float rotation) {
        rotation_ = rotation;
    }

    Vector4& GetColor() const {
        return materialData_->color;
    }
    void SetColor(const Vector4& color) {
        materialData_->color = color;
    }

    Matrix4x4& GetUV()const {
        return materialData_->uvTransform;
    }
    void SetUV(Matrix4x4& uvTransform) {
        materialData_->uvTransform = uvTransform;
    }

    const Vector2& GetSize()const {
        return size_;
    }
    void SetSize(const Vector2& Size) {
        this->size_ = Size;
    }

    const Vector2& GetAnchorPoint()const {
        return anchorPoint_;
    }
    void SetAnchorPoint(const Vector2& anchorPoint) {
        anchorPoint_ = anchorPoint;
    }

    bool GetIsFlipX()const {
        return isFlipX_;
    }
    void SetIsFlipX(bool isFlipX) {
        isFlipX_ = isFlipX;
    }
    bool GetIsFlipY()const {
        return isFlipY_;
    }
    void SetIsFlipY(bool isFlipY) {
        isFlipY_ = isFlipY;
    }

    Vector2 GetTextureLeftTop()const {
        return textureLeftTop;
    }
    void SetTextureLeftTop(const Vector2& textureLeftTop) {
        this->textureLeftTop = textureLeftTop;
    }
    Vector2 GetTextureSize()const {
        return textureSize;
    }
    void SetTextureSize(const Vector2& textureSize) {
        this->textureSize = textureSize;
    }
    void SetBlendMode(BlendMode blendMode) {
        blendMode_ = blendMode;
    }
    BlendMode GetBlendMode() const {
        return blendMode_;
    }
    void SetFillMode(FillMode fillMode) { fillMode_ = fillMode; }
    //繝・け繧ｹ繝√Ε螟画峩
    void SetTextureByFilePath(const std::string& textureFilePath);

private:
    void AdjustTextureSize();
    BlendMode blendMode_ = BlendMode::Normal;
private:
    

    Vector2 position_ = { 0.0f,0.0f };
    float rotation_ = 0.0f;

    Vector2 size_ = { 10.0f,10.0f };

    Vector2 anchorPoint_ = { 0.0f,0.0f };

    bool isFlipX_ = false;
    bool isFlipY_ = false;

    //繝・け繧ｹ繝√Ε蟾ｦ荳・
    Vector2 textureLeftTop = { 0.0f,0.0f };
    //繝・け繧ｹ繝√Ε蛻・ｊ蜃ｺ縺励し繧､繧ｺ
    Vector2 textureSize{ 100.0f,100.0f };


    //buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourse_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    VertexData* vertexData_ = nullptr;
    uint32_t* indexData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    //繝槭ユ繝ｪ繧｢繝ｫ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
    //蠎ｧ讓吝､画鋤
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourse_;
    TransformationMatrix* transformationMatrixData_ = nullptr;
    uint32_t textureIndex_ = 0;
    std::string textureFilePath_;
    FillMode fillMode_ = FillMode::kSolid;
};

