#include "common.hlsl"





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

struct PSInput
{
    float4 position      : SV_POSITION;
    float3 color         : COLOR;
    float2 uv            : TEXCOORD0;
    float3 worldNormal   : NORMAL;
    float3 worldPosition : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 baseColor = sampleTextureLinear(g_textures[pc.diffuseTextureBindlessTextureArraySlot], uv).rgb;
    float diff = saturate(dot(input.worldNormal, -pc.directionalLight.xyz));
    baseColor *= diff;
    return float4(baseColor, 1.0f);
}
