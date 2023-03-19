
add_subdirectory(Core)
add_subdirectory(AssetLibrary)

find_package(Vulkan REQUIRED)

add_executable(vkx
    "./Window/Window.h"
    "./Window/Window.cpp"
    "./Main.cpp"
    "./Engine.h"
    "./Engine.cpp"
    "./VulkanCommon.h"
    "./VulkanCommon.cpp"
    "./Mesh.h"
    "./Mesh.cpp"
    "./Input.h"
    "./Input.cpp"
    "./ECS.h"
    "./ECS.cpp"
    )

if (UNIX)
    find_package(X11 REQUIRED)

    target_sources(vkx PRIVATE
        "./Window/GLFWWindow.h"
        "./Window/GLFWWindow.cpp"
        )
    target_link_libraries(vkx PRIVATE glfw ${X11_LIBRARIES})
    target_include_directories(vkx PRIVATE ${X11_INCLUDE_DIRS})
elseif(WIN32)

    target_sources(vkx PUBLIC
        "./Window/Win32Window.h"
        "./Window/Win32Window.cpp"
        "./Window/Win32ImGUI.h"
        "./Window/Win32ImGUI.cpp"
        "./Window/Win32Vulkan.h"
        "./Window/Win32Vulkan.cpp"
        )
    target_link_libraries(vkx PRIVATE "user32")
endif()

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
    )
