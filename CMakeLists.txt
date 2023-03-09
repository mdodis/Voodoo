cmake_minimum_required(VERSION 3.7.1)
project(VKX)
set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RIGHT_HANDED
    )

set(MOKLIB_COMPILE_STATIC ON CACHE BOOL "Compile MokLib as static library")
add_subdirectory("ThirdParty/ImGUI")
add_subdirectory("ThirdParty/VMA")
add_subdirectory("ThirdParty/TinyOBJLoader")
add_subdirectory("ThirdParty/STB")
add_subdirectory("ThirdParty/glm")
add_subdirectory("MokLib")
add_subdirectory("Shaders")
add_subdirectory("Engine")