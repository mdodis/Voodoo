
add_subdirectory(Core)
add_subdirectory(AssetLibrary)

find_package(Vulkan REQUIRED)

add_executable(vkx
    "./Main.cpp"
    "./Window.cpp"
    "./Window.h"
    "./Engine.h"
    "./Engine.cpp"
    "./VulkanCommon.h"
    "./VulkanCommon.cpp"
    "./Mesh.h"
    "./Mesh.cpp"
    "./Input.h"
    "./Input.cpp"
    "./Win32ImGui.h"
    "./Win32ImGui.cpp"
    "./ECS.h"
    "./ECS.cpp"
    )

target_include_directories(vkx 
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/ThirdParty/VMA/
    "./"
    )
    
target_link_libraries(vkx
    PRIVATE
    core
    ${Vulkan_LIBRARY}
    VulkanMemoryAllocator
    tinyobjloader
    imgui
    flecs
    stb_image
    assetlib
    "user32"
    )