#version 450 core
// Modified fragment shader to output colors in linear space
vec3 srgbToLinear(vec3 color)
{
    bvec3 useLinearSegment = lessThanEqual(color, vec3(0.04045));
    vec3 linearSegment = color / 12.92;
    vec3 exponentialSegment = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(exponentialSegment, linearSegment, useLinearSegment);
}


layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    vec4 linearVertexColor = vec4(srgbToLinear(In.Color.rgb), In.Color.a);
    fColor = linearVertexColor * texture(sTexture, In.UV.st);
}