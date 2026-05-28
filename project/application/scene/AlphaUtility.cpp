#include "AlphaUtility.h"


// 経過時間 time [秒] を渡すと 0.0〜1.0 のアルファ値を返す
float BlinkAlpha(float time, float speed)
{
    // sin は -1〜1 なので 0〜1 に変換
    return (std::sin(time * speed * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f;
}

float BlinkAlpha(float time, float speed, float minA, float maxA)
{
    float t = (std::sin(time * speed * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f; // 0〜1
    return minA + t * (maxA - minA); // lerp
}

float GlowAlpha(float time, float speed)
{
    float s = (std::sin(time * speed * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f;
    return s * s; // 暗い時間が長く、明るい瞬間が短い
}

float FadeAlpha(float time, float speed)
{
    float s = (std::sin(time * speed * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f;
    return std::sqrt(s); // 明るい時間が長く、暗い瞬間が短い
}
