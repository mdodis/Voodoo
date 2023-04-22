cmake_minimum_required(VERSION 3.7.1)

set(INCLUDE_DIRS "./")
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(imguizmo STATIC
    ${SOURCES}
    ${HEADERS}
    )

target_link_libraries(imguizmo PUBLIC
    imgui)

target_include_directories(imguizmo
    PUBLIC
    ${INCLUDE_DIRS}
    )

target_compile_definitions(imguizmo PRIVATE
    "IMGUI_DEFINE_MATH_OPERATORS")