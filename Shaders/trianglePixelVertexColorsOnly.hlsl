



[[vk::binding(0, 0)]] // Set 0, binding 0
SamplerState g_samplers[2]; // Samplers

[[vk::binding(1, 0)]] // Set 0, binding 1
Texture2D g_textures[]; // Texture array

float4 sampleTextureLinear(Texture2D tex, float2 texCoords) {
    return tex.Sample(g_samplers[0], texCoords);
}

float4 sampleTexturePoint(Texture2D tex, float2 texCoords) {
    return tex.Sample(g_samplers[1], texCoords);
}

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
    float2 uv = input.uv;
    return float4(input.color, 1.0f);
}
