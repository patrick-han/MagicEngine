
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};


VSOutput main(uint vertexID : SV_VertexID)
{
    const float2 positions[3] =
    {
        float2( 0.0f, -0.5f),
        float2( 0.5f,  0.5f),
        float2(-0.5f,  0.5f)
    };

    const float3 colors[3] =
        {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 2.0f, 0.0f),
        float3(0.0f, 0.0f, 3.0f)
    };

    VSOutput output;
    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    output.color = colors[vertexID];
    return output;
}