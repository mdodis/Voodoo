cmake_minimum_required(VERSION 3.7.1)

set(INCLUDE_DIRS "./")

add_library(imgui STATIC
    "./imconfig.h"
    "./imgui_internal.h"
    "./imgui.h"
    "./imstb_rectpack.h"
    "./imstb_textedit.h"
    "./imstb_truetype.h"
    "./imgui_demo.cpp"
    "./imgui_draw.cpp"
    "./imgui_tables.cpp"
    "./imgui_widgets.cpp"
    "./imgui.cpp"
    "./backends/imgui_impl_vulkan.h"
    "./backends/imgui_impl_vulkan.cpp"
    )

if (WIN32)
    target_sources(imgui PRIVATE
        "./backends/imgui_impl_win32.h"
        "./backends/imgui_impl_win32.cpp"
        )
    target_link_libraries(imgui
        PUBLIC
        "user32"
        )
elseif(UNIX)
    target_sources(imgui PRIVATE
        "./backends/imgui_impl_glfw.h"
        "./backends/imgui_impl_glfw.cpp"
        )
    target_link_libraries(imgui
        PUBLIC
        glfw
        )
endif()

find_package(Vulkan)
target_include_directories(imgui
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${INCLUDE_DIRS}
    )

target_link_libraries(imgui
    PUBLIC
    ${Vulkan_LIBRARY}
    )
