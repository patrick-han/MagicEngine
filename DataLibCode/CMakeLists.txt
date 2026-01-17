add_subdirectory(ThirdParty)
add_executable(DataCooker main.cpp
        ../CommonCode/Math/Vector3f.h
        ../CommonCode/Math/Vector3f.cpp
        ../CommonCode/Log.h
        ../CommonCode/Log.cpp
        ../CommonCode/Math/Matrix4f.h
        ../CommonCode/Math/Matrix4f.cpp
        ../CommonCode/Math/Vector4f.h
        ../CommonCode/Math/Vector4f.cpp
        DataSerialization.h
        ../EngineCode/Vertex.h
        ../EngineCode/Model.h
        stb_image.h
        ../CommonCode/Math/Vector2f.h
        ImportGLTF.cpp
)

target_link_libraries(DataCooker PRIVATE DataLibThirdPartyLibraries)