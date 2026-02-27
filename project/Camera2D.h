#pragma once
#pragma once
#include "Vector2.h"
#include "Vector4.h"
#include "WinApp.h"
class Camera2D
{
    public:
    Camera2D();
    void Update();

    // Setter
    void SetPosition(const Vector2& pos) { position_ = pos; }
    void SetRotation(float rot) { rotation_ = rot; }
    void SetScale(const Vector2& scale) { scale_ = scale; }

    Vector2 GetPosition()const { return position_;  }
    float GetRotation()const {return rotation_ ; }
    Vector2 GetScale()const {return scale_ ; }

    // Getter
    const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
    const Matrix4x4& GetViewProjectionMatrix() const { return vpMatrix_; }
private:
    Vector2 position_ = { 0.0f, 0.0f };
    float rotation_ = 0.0f;
    Vector2 scale_ = { 1.0f, 1.0f };

    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Matrix4x4 vpMatrix_;
};

