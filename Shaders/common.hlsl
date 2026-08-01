struct PushConstants
{
    row_major float4x4 modelMatrix;
    row_major float4x4 viewProjectionMatrix;
    float4 directionalLight;
    uint diffuseTextureBindlessTextureArraySlot;
    uint normalTextureBindlessTextureArraySlot;
    uint metallicRoughnessTextureBindlessTextureArraySlot;
    float metallicFactor;
    float roughnessFactor;
    float normalYSign;
};

[[vk::push_constant]] PushConstants pc;
