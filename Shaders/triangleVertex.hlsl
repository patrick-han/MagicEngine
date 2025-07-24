
struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION; // semantic matches vertex input layout
    [[vk::location(1)]] float3 color    : COLOR;    // semantic matches vertex input layout
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

struct PushConstants
{
    row_major float4x4 modelMatrix;
    row_major float4x4 viewProjectionMatrix;
};

[[vk::push_constant]] PushConstants pc;

//VSOutput main(uint vertexID : SV_VertexID)
VSOutput main(VSInput input)
{
    VSOutput output;
    // output.position = float4(input.position, 1.0f);
    float4 worldPosition = mul(pc.modelMatrix, float4(input.position, 1.0f));
    output.position = mul(pc.viewProjectionMatrix, worldPosition);
    output.color = input.color;
    return output;
}