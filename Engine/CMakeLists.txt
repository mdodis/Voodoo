
add_subdirectory(Core)
add_subdirectory(AssetLibrary)

find_package(Vulkan REQUIRED)

add_executable(vkx
    "./Window/Window.h"
    "./Window/Window.cpp"
    "./Main.cpp"
    "./PipelineBuilder.h"
    "./PipelineBuilder.cpp"
    "./EngineTypes.h"
    "./Engine.h"
    "./Engine.cpp"
    "./VulkanCommon.h"
    "./VulkanCommon.cpp"
    "./DescriptorAllocator.h"
    "./DescriptorAllocator.cpp"
    "./DescriptorLayoutCache.h"
    "./DescriptorLayoutCache.cpp"
    "./DescriptorBuilder.h"
    "./DescriptorBuilder.cpp"
    "./Mesh.h"
    "./Mesh.cpp"
    "./Input.h"
    "./Input.cpp"
    "./ECS.h"
    "./ECS.cpp"
    "./ECSTypes.h"
    "./ECSTypes.cpp"
    "./WorldSerializer.h"
    "./WorldSerializer.cpp"
    "./RenderObject.h"
    "./VMA.h"
    "./VMA.cpp"
    "./Editor/Editor.h"
    "./Editor/Editor.cpp"
    "./Editor/Gizmos.h"
    "./Editor/Gizmos.cpp"
    "./Editor/MenuRegistrar.h"
    "./Editor/MenuRegistrar.cpp"
    "./Editor/HierarchyEditorWindow.h"
    "./Editor/HierarchyEditorWindow.cpp"
    "./Editor/InspectorEditorWindow.h"
    "./Editor/InspectorEditorWindow.cpp"
    "./Editor/ModelToVertexArrayEditorWindow.h"
    "./Editor/ModelToVertexArrayEditorWindow.cpp"
    "./Editor/ColorScheme.h"
    "./ImmediateDrawQueue.h"
    "./ImmediateDrawQueue.cpp"
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
    set_property(TARGET vkx PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    target_sources(vkx PUBLIC
        "./Window/Win32Window.h"
        "./Window/Win32Window.cpp"
        "./Window/Win32DnD.h"
        "./Window/Win32DnD.cpp"
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
    imgui
    flecs
    assetlib
    portable_file_dialogs
    )
