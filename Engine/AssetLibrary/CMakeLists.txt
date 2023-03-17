file(GLOB SOURCES "*.cpp" "*.h" "Importers/*.cpp")

add_library(assetlib STATIC ${SOURCES})

target_include_directories(assetlib PRIVATE "./")

target_link_libraries(assetlib
    PUBLIC
    core
    PRIVATE
    LZ4
    stb_image)

add_subdirectory(Tests)