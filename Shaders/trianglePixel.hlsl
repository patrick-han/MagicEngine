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

float3 decodeNormal(float3 normal)
{
    normal = normal * 2.0 - 1.0;
    normal.y *= pc.normalYSign; // needs to happen before world space transform
    return normal;
}

struct PSInput
{
    float4 position      : SV_POSITION;
    float3 color         : COLOR;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
    float3 T             : TEXCOORD2;
    float3 B             : TEXCOORD3;
    float3 N             : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
    float3x3 tangentToWorldSpace = transpose(float3x3(input.T, input.B, input.N));
    float2 uv = input.uv;
    float3 sampledAlbedo = sampleTextureLinear(g_textures[pc.diffuseTextureBindlessTextureArraySlot], uv).rgb;
    float3 sampledNormal = sampleTextureLinear(g_textures[pc.normalTextureBindlessTextureArraySlot], uv).rgb;
    sampledNormal = decodeNormal(sampledNormal);
    float3 sampledNormalWS = normalize(mul(tangentToWorldSpace, sampledNormal));

    float3 sampledMetallicRoughness = sampleTextureLinear(g_textures[pc.metallicRoughnessTextureBindlessTextureArraySlot], uv).rgb;
    float sampledMetallic = sampledMetallicRoughness.b * pc.metallicFactor;
    float sampledRoughness = sampledMetallicRoughness.g * pc.roughnessFactor;



    float diff = saturate(dot(sampledNormalWS, -pc.directionalLight.xyz));
    sampledAlbedo *= diff;
    return float4(sampledAlbedo, 1.0);
}
