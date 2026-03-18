#include "SpiderWeb.hlsli"

// ★追加：C++側で t0 にセットしたテクスチャを受け取る
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    // ★変更：テクスチャの色をサンプリング（抽出）して、マテリアルの色(color)と掛け合わせる
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    return color * texColor;
}