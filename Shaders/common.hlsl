struct DirectionalLightData
{
    float4 direction;
    float4 color;

    float angle;
    float intensity;
    float exposure;
    float data0;
};

struct WorldData
{
    DirectionalLightData dirLight;
};


struct PushConstants
{
    row_major float4x4 modelMatrix;
    row_major float4x4 viewProjectionMatrix;

    uint diffuseTextureBindlessTextureArraySlot;
    uint normalTextureBindlessTextureArraySlot;
    uint metallicRoughnessTextureBindlessTextureArraySlot;

    uint64_t worldDataBufferAddress;

    float metallicFactor;
    float roughnessFactor;
    float normalYSign;
};

[[vk::push_constant]] PushConstants pc;

WorldData GetWorldData()
{
    return vk::RawBufferLoad<WorldData>(pc.worldDataBufferAddress);
}
