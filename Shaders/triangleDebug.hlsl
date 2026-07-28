#include "common.hlsl"

struct PSInput
{
    float4 position      : SV_POSITION;
    float3 color         : COLOR;
    float2 uv            : TEXCOORD0;
    float3 worldNormal        : NORMAL;
    float3 worldPosition : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}
