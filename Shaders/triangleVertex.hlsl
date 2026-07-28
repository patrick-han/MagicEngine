
#include "common.hlsl"

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION; // semantic matches vertex input layout
    [[vk::location(1)]] float  uv_x     : TEXCOORD0;
    [[vk::location(2)]] float3 color    : COLOR;    // semantic matches vertex input layout
    [[vk::location(3)]] float  uv_y     : TEXCOORD1;
    [[vk::location(4)]] float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float3 color         : COLOR;
    float2 uv            : TEXCOORD0;
    float3 worldNormal   : NORMAL;
    float3 worldPosition : TEXCOORD1;
};

//VSOutput main(uint vertexID : SV_VertexID)
VSOutput main(VSInput input)
{
    VSOutput output;
    // output.position = float4(input.position, 1.0f);
    float4 worldPosition = mul(pc.modelMatrix, float4(input.position, 1.0f));
    output.position = mul(pc.viewProjectionMatrix, worldPosition);
    output.color = input.color;
    output.uv = float2(input.uv_x, input.uv_y);
    output.worldNormal = normalize(mul((float3x3)pc.modelMatrix, input.normal)); // Assumes world transform mtx is rotation, translation, and uniform scaling
    output.worldPosition = worldPosition.xyz;
    return output;
}
