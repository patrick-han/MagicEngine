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


float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

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
    float sampledRoughness = clamp(sampledMetallicRoughness.g * pc.roughnessFactor, 0.045, 1.0);

    WorldData worldData = GetWorldData();

    float3 N = sampledNormalWS;
    float3 V = normalize(worldData.cameraPos.xyz - input.worldPosition);

    float3 Lo = float3(0.0, 0.0, 0.0);
    
    // Directional light contribution
    float3 L = normalize(-worldData.dirLight.direction.xyz);
    float3 H = normalize(V + L);
    float attenuation = 1.0;
    float lightBrightness = worldData.dirLight.intensity * exp2(worldData.dirLight.exposure);
    float3 radiance = lightBrightness * worldData.dirLight.color.rgb * attenuation;
    float3 F0 = float3(0.04, 0.04, 0.04); // simplified assumption that most dielectric surfaces look visually 'correct' with a constant F0 of 0.04
    F0 = lerp(F0, sampledAlbedo, sampledMetallic);

    // BRDF
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(N, H, sampledRoughness);
    float G   = GeometrySmith(N, V, L, sampledRoughness);
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // prevent divide by 0 in case dot products are 0
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - sampledMetallic;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * sampledAlbedo / PI + specular) * radiance * NdotL;


    float3 ambient = float3(0.03, 0.03, 0.03) * sampledAlbedo; // improvised ambient term
    // float3 ambient = 0.03 * sampledAlbedo * (1.0 - sampledMetallic);
    float3 color = ambient + Lo;

    return float4(color * input.color, 1.0);
}
