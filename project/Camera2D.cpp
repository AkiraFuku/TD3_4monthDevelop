#include "Camera2D.h"
#include "MathFunction.h"

Camera2D::Camera2D() {
    Update();
}

void Camera2D::Update() {
    // 2D用のビュー行列（カメラの逆変換）
    // アフィン変換(Scale, Rotate, Translate)の逆行列を作成
    Transform transform = { 
        {scale_.x, scale_.y, 1.0f}, 
        {0.0f, 0.0f, rotation_}, 
        {position_.x, position_.y, 0.0f} 
    };
    Matrix4x4 worldMatrix = MakeAfineMatrix(transform.scale, transform.rotate, transform.translate);
    viewMatrix_ = Inverse(worldMatrix);

    // 平行投影行列の作成（画面サイズに合わせる）
    projectionMatrix_ = MakeOrthographicMatrix(
        0.0f, 0.0f, 
        static_cast<float>(WinApp::kClientWidth), 
        static_cast<float>(WinApp::kClientHeight), 
        0.0f, 100.0f
    );
    vpMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}
