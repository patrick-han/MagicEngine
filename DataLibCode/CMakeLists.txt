add_subdirectory(ThirdParty)
add_executable(DataCooker main.cpp
        DataSerialization.h
        ../EngineCode/Vertex.h
        ../EngineCode/Model.h
        stb_image.h
        ImportGLTF.cpp
)

target_link_libraries(DataCooker PRIVATE DataLibThirdPartyLibraries)
target_link_libraries(DataCooker PRIVATE CommonLibrary)