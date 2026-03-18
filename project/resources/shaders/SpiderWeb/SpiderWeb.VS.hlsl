#include "SpiderWeb.hlsli"

// ★ C++側で t1 に設定したので register(t1) に修正
StructuredBuffer<InstanceData> gInstanceData : register(t1);

struct VSInput
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD; // ★追加：C++の頂点レイアウトに合わせる
    float3 normal : NORMAL;
};

VSOutput main(VSInput input, uint instanceId : SV_InstanceID)
{
    VSOutput output;
    matrix worldMat = gInstanceData[instanceId].World;
    
    float4 worldPos = mul(input.pos, worldMat);
    
    output.pos = mul(worldPos, gTransformation.WVP);
    output.normal = mul(input.normal, (float3x3) worldMat);
    
    // ★追加：UV座標をそのままピクセルシェーダーへ送る
    output.uv = input.uv;
    
    return output;
}