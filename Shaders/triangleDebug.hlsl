struct PushConstants
{
    row_major float4x4 modelMatrix;
    row_major float4x4 viewProjectionMatrix;
    uint diffuseTextureBindlessTextureArraySlot;
};

[[vk::push_constant]] PushConstants pc;

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}
