#include "WorldToScreen.h"

Vector4 Vector4Transform(const Vector4& v, const Matrix4x4& m)
{
    return Vector4(
       m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
       m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
       m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
       m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w
    );
}

ScreenPosition WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewMatrix, 
    const Matrix4x4& projMatrix, int32_t screenWidth, int32_t screenHeight)
{
    ScreenPosition result;

    // -----------------------------------------------------------------------
    // Step 1: ワールド座標を Vec4 に変換 (w=1.0 で位置ベクトルとして扱う)
    // -----------------------------------------------------------------------
    Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);

    // -----------------------------------------------------------------------
    // Step 2: ビュー変換 (ワールド空間 → カメラ空間)
    //         viewMatrix はカメラの逆行列
    // -----------------------------------------------------------------------
    Vector4 viewPos = Vector4Transform(pos, viewMatrix);

    // -----------------------------------------------------------------------
    // Step 3: プロジェクション変換 (カメラ空間 → クリップ空間)
    //         結果は Vec4(x, y, z, w) のクリップ座標
    // -----------------------------------------------------------------------
    Vector4 clipPos = Vector4Transform(viewPos, projMatrix);

    // -----------------------------------------------------------------------
    // Step 4: カメラの後方チェック
    //         w <= 0 の場合、オブジェクトはカメラ後方にあるためスキップ
    // -----------------------------------------------------------------------
    if (clipPos.w <= 0.0f)
    {
        result.isVisible = false;
        return result;
    }

    // -----------------------------------------------------------------------
    // Step 5: 透視除算 (クリップ空間 → NDC)
    //         NDC の範囲: x, y ともに [-1.0, 1.0]
    // -----------------------------------------------------------------------
    float ndcX = clipPos.x / clipPos.w;
    float ndcY = clipPos.y / clipPos.w;

    // -----------------------------------------------------------------------
    // Step 6: 可視範囲チェック (NDC が [-1, 1] を超えていればスクリーン外)
    //         マージンを少し設けることで、スクリーン端付近のオブジェクトも扱える
    // -----------------------------------------------------------------------
    const float margin = 1.0f; // 1.0f = スクリーンちょうど端。大きくするとスクリーン外も許容
    if (ndcX < -margin || ndcX > margin || ndcY < -margin || ndcY > margin)
    {
        result.isVisible = false;
        return result;
    }

    // -----------------------------------------------------------------------
    // Step 7: NDC → スクリーン座標 (ピクセル)
    //         NDC: 左=-1, 右=+1, 上=+1, 下=-1
    //         Screen: 左=0, 右=Width, 上=0, 下=Height
    //
    //   screenX = (ndcX + 1.0) * 0.5 * screenWidth
    //   screenY = (1.0 - ndcY) * 0.5 * screenHeight   ← Y軸を反転
    // -----------------------------------------------------------------------
    result.position.x = (ndcX + 1.0f) * 0.5f * screenWidth;
    result.position.y = (1.0f - ndcY) * 0.5f * screenHeight;
    result.isVisible = true;

    return result;
}