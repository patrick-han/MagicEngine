

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION; // semantic matches vertex input layout
    [[vk::location(1)]] float  uv_x     : TEXCOORD0;
    [[vk::location(2)]] float3 color    : COLOR;    // semantic matches vertex input layout
    [[vk::location(3)]] float  uv_y     : TEXCOORD1;
    [[vk::location(4)]] float3 normal   : NORMAL;
    [[vk::location(5)]] float  data     : TEXCOORD2;
    [[vk::location(6)]] float4 tangentW : TEXCOORD3;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    // float3 worldPosition : TEXCOORD0;
};

struct ShadowPushConstants
{
    row_major float4x4 modelMatrix;
    row_major float4x4 shadowViewProjectionMatrix;
};

[[vk::push_constant]] ShadowPushConstants pc;

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = mul(pc.modelMatrix, float4(input.position, 1.0f));
    output.position = mul(pc.shadowViewProjectionMatrix, worldPosition);
    // output.worldPosition = worldPosition.xyz;
    return output;
}
