file(GLOB SOURCES "*.cpp" "*.h" "Importers/*.cpp")

add_library(AssetLibrary STATIC ${SOURCES})

target_include_directories(AssetLibrary
    PRIVATE "./"
    PUBLIC "../")

target_link_libraries(AssetLibrary
    PUBLIC
    core
    glm
    PRIVATE
    LZ4
    stb_image
    tinyobjloader
    Tracy::TracyClient)

add_subdirectory(Tests)