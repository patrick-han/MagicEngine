dxc -spirv -T vs_6_0 -E main ./Shaders/triangleVertex.hlsl -Fo ./Shaders/triangleVertex.vertex.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/trianglePixel.hlsl -Fo ./Shaders/trianglePixel.pixel.spv
dxc -spirv -T ps_6_0 -E main ./Shaders/triangleDebug.hlsl -Fo ./Shaders/triangleDebug.pixel.spv