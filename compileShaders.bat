dxc -spirv -T vs_6_0 -E main ./Shaders/triangleVertex.hlsl -Fo ./Shaders/triangleVertex.vertex.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/trianglePixel.hlsl -Fo ./Shaders/trianglePixel.pixel.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/triangleDebug.hlsl -Fo ./Shaders/triangleDebug.pixel.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/trianglePixelVertexColorsOnly.hlsl -Fo ./Shaders/trianglePixelVertexColorsOnly.pixel.spv

dxc -spirv -T vs_6_0 -E main ./Shaders/aabbVertex.hlsl -Fo ./Shaders/aabbVertex.vertex.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/aabbPixel.hlsl -Fo ./Shaders/aabbPixel.pixel.spv