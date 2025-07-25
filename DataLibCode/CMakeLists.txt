add_subdirectory(ThirdParty)
add_executable(DataCooker main.cpp
        Import.h
        ../EngineCode/Common/Math/Vector3f.h
        ../EngineCode/Common/Math/Vector3f.cpp
        ../EngineCode/Common/Log.h
        ../EngineCode/Common/Log.cpp
        ../EngineCode/Common/Math/Matrix4f.h
        ../EngineCode/Common/Math/Matrix4f.cpp
        ../EngineCode/Common/Math/Vector4f.h
        ../EngineCode/Common/Math/Vector4f.cpp
        Data.h
        ../EngineCode/Vertex.h
        ../EngineCode/Mesh.h
        ../EngineCode/Model.h
)

target_link_libraries(DataCooker PRIVATE DataLibThirdPartyLibraries)