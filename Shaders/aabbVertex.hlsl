struct PushConstants
{
    float3 aabbMin;
    float pad0;
    float3 aabbMax;
    float pad1;
    row_major float4x4 viewProjection;
};

[[vk::push_constant]] PushConstants pc;



struct VSOutput
{
    float4 pos : SV_Position;
};

// Each pair must differ in exactly one axis bit
static const uint2 EDGE_CORNER[12] = {
    // bottom (z = 0): go around without crossing
    uint2(0,1), uint2(1,3), uint2(3,2), uint2(2,0),

    // top (z = 1)
    uint2(4,5), uint2(5,7), uint2(7,6), uint2(6,4),

    // verticals
    uint2(0,4), uint2(1,5), uint2(3,7), uint2(2,6)
};

// Corner decode from bits (x=LSB, y=next, z=MSB)
float3 corner(uint id, float3 mn, float3 mx) {
    return float3(
        (id & 1) ? mx.x : mn.x,
        (id & 2) ? mx.y : mn.y,
        (id & 4) ? mx.z : mn.z
    );
}

VSOutput main(uint vid : SV_VertexID) {
    uint e = vid >> 1;       // 0..11
    uint ep = vid & 1;       // 0 or 1
    uint cid = ep ? EDGE_CORNER[e].y : EDGE_CORNER[e].x;
    float3 p = corner(cid, pc.aabbMin, pc.aabbMax);

    VSOutput o;
    o.pos = mul(pc.viewProjection, float4(p, 1.0));
    // o.world = wpos.xyz;
    return o;
}