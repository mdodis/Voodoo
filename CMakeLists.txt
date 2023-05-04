cmake_minimum_required(VERSION 3.7.1)
project(VKX)
set(CMAKE_CXX_STANDARD 20)
include(CTest)

add_compile_definitions(
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RIGHT_HANDED
    METADESK=0
    )

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "glfw")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "glfw")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "glfw")
set(GLFW_INSTALL OFF CACHE BOOL "glfw")

set(MOKLIB_COMPILE_STATIC ON CACHE BOOL "Compile MokLib as static library")
add_subdirectory("ThirdParty/flecs")
add_subdirectory("ThirdParty/glm")
add_subdirectory("ThirdParty/SPIRVReflect")
add_subdirectory("ThirdParty/GLFW")
add_subdirectory("ThirdParty/ImGUI")
add_subdirectory("ThirdParty/LZ4")
add_subdirectory("ThirdParty/PortableFileDialogs")
add_subdirectory("ThirdParty/STB")
add_subdirectory("ThirdParty/TinyOBJLoader")
add_subdirectory("ThirdParty/VMA")
add_subdirectory("ThirdParty/Metadesk")
add_subdirectory("Metacompiler")
add_subdirectory("MokLib")
add_subdirectory("Shaders")
add_subdirectory("Source")
# add_subdirectory("Engine")
