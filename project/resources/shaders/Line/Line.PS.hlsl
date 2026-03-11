#include "Line.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float4 main(float4 color : COLOR) : SV_TARGET0
{
    return color;
}