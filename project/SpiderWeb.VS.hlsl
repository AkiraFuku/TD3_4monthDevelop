#include "SpiderWeb.hlsli"

StructuredBuffer<InstanceData> gInstanceData : register(t0);

struct VSInput
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
};

VSOutput main(VSInput input, uint instanceId : SV_InstanceID)
{
    VSOutput output;
    matrix worldMat = gInstanceData[instanceId].World;
    
    float4 worldPos = mul(input.pos, worldMat);
    
    // ★ gTransformation.WVP を使う（中身にはカメラの ViewProjection を入れる）
    output.pos = mul(worldPos, gTransformation.WVP);
    output.normal = mul(input.normal, (float3x3) worldMat);
    
    return output;
}