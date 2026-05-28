#pragma once

#include "MathFunction.h"
#include "Vector4.h"

struct ScreenPosition
{
    Vector2 position; // スクリーン上の座標
    bool  isVisible = false; // スクリーン内に映っているか
};

Vector4 Vector4Transform(const Vector4& vector, const Matrix4x4& matrix);

/**
 * ワールド座標 → スクリーン座標 変換
 *
 * @param worldPos      変換したいワールド空間の座標
 * @param viewMatrix    カメラのビュー行列
 * @param projMatrix    プロジェクション行列 (透視投影)
 * @param screenWidth   スクリーン幅 (ピクセル)
 * @param screenHeight  スクリーン高さ (ピクセル)
 * @return              スクリーン座標と可視性フラグ
 */
ScreenPosition WorldToScreen(
    const Vector3& worldPos,
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projMatrix,
    int32_t             screenWidth,
    int32_t             screenHeight);