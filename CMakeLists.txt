cmake_minimum_required(VERSION 3.7.1)
project(VKX)
set(CMAKE_CXX_STANDARD 20)
include(CTest)

# Enable profiler support
option(TRACY_ENABLE "" OFF)

# Library specific options
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

option(TRACY_STATIC "" ON)

# Add a test with the default properties already specified
function(v_add_test test_target)
    add_test(
        NAME ${test_target}_test
        COMMAND ${test_target}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_property(TEST ${test_target}_test PROPERTY PASS_REGULAR_EXPRESSION "Passed")
    set_property(TEST ${test_target}_test PROPERTY FAIL_REGULAR_EXPRESSION "Failed")
endfunction()

add_subdirectory("ThirdParty/SFL")
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
add_subdirectory("ThirdParty/Tracy")
add_subdirectory("Metacompiler")
add_subdirectory("MokLib")
add_subdirectory("Shaders")
add_subdirectory("Source")