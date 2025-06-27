
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

//VSOutput main(uint vertexID : SV_VertexID)
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}