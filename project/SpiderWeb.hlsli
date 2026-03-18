struct InstanceData
{
    matrix World;
};

cbuffer Material : register(b0)
{
    float4 color;
};

struct TransformationMatrix
{
    matrix WVP;
    matrix World;
    matrix WorldInverseTranspose;
};

cbuffer Transformation : register(b1)
{
    TransformationMatrix gTransformation;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};