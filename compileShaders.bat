dxc -spirv -I ./Shaders -T vs_6_0 -E main ./Shaders/shadowVertex.hlsl -Fo ./Shaders/shadowVertex.vertex.spv

dxc -spirv -I ./Shaders -T vs_6_0 -E main ./Shaders/triangleVertex.hlsl -Fo ./Shaders/triangleVertex.vertex.spv
dxc -spirv -I ./Shaders -T ps_6_0 -E main ./Shaders/trianglePixel.hlsl -Fo ./Shaders/trianglePixel.pixel.spv

dxc -spirv -I ./Shaders -T vs_6_0 -E main ./Shaders/aabbVertex.hlsl -Fo ./Shaders/aabbVertex.vertex.spv
dxc -spirv -I ./Shaders -T ps_6_0 -E main ./Shaders/aabbPixel.hlsl -Fo ./Shaders/aabbPixel.pixel.spv

glslangValidator -V ./Shaders/imgui.vert -o ./Shaders/imgui.vertex.spv
glslangValidator -V ./Shaders/imgui.frag -o ./Shaders/imgui.pixel.spv
