
add_subdirectory(Core)
add_subdirectory(AssetLibrary)

find_package(Vulkan REQUIRED)

set(HEADERS
    "./Window/Window.h"
    "./PipelineBuilder.h"
    "./EngineTypes.h"
    "./Engine.h"
    "./VulkanCommon.h"
    "./DescriptorAllocator.h"
    "./DescriptorLayoutCache.h"
    "./DescriptorBuilder.h"
    "./Mesh.h"
    "./Input.h"
    "./ECS.h"
    "./ECS.h"
    "./ECSTypes.h"
    "./WorldSerializer.h"
    "./RenderObject.h"
    "./VMA.h"
    "./Editor/Editor.h"
    "./Editor/Gizmos.h"
    "./Editor/MenuRegistrar.h"
    "./Editor/HierarchyEditorWindow.h"
    "./Editor/InspectorEditorWindow.h"
    "./Editor/WorldViewEditorWindow.h"
    "./Editor/ModelToVertexArrayEditorWindow.h"
    "./Editor/QueryEditorWindow.h"
    "./Editor/ColorScheme.h"
    "./ImmediateDrawQueue.h"
    "./Window/GLFWWindow.h"
    "./Window/Win32Window.h"
    "./Window/Win32DnD.h"
    "./Window/Win32ImGUI.h"
    "./Window/Win32Vulkan.h"
    "./TestSystem.h"
    "./SystemDescriptor.h"
    "./TransformSystem.h"
    )


add_executable(vkx
    ${HEADERS}
    "./Window/Window.cpp"
    "./Main.cpp"
    "./PipelineBuilder.cpp"
    "./Engine.cpp"
    "./VulkanCommon.cpp"
    "./DescriptorAllocator.cpp"
    "./DescriptorLayoutCache.cpp"
    "./DescriptorBuilder.cpp"
    "./Mesh.cpp"
    "./Input.cpp"
    "./ECS.cpp"
    "./ECSTypes.cpp"
    "./WorldSerializer.cpp"
    "./VMA.cpp"
    "./Editor/Editor.cpp"
    "./Editor/Gizmos.cpp"
    "./Editor/MenuRegistrar.cpp"
    "./Editor/HierarchyEditorWindow.cpp"
    "./Editor/InspectorEditorWindow.cpp"
    "./Editor/WorldViewEditorWindow.cpp"
    "./Editor/ModelToVertexArrayEditorWindow.cpp"
    "./Editor/QueryEditorWindow.cpp"
    "./ImmediateDrawQueue.cpp"
    "./SystemDescriptor.cpp"
    "${PROJECT_SOURCE_DIR}/Intermediate/Engine/Module.cpp"
    "./TestSystem.cpp"
    "./TransformSystem.cpp"
    )

if (UNIX)
    find_package(X11 REQUIRED)

    target_sources(vkx PRIVATE

        "./Window/GLFWWindow.cpp"
        )
    target_link_libraries(vkx PRIVATE glfw ${X11_LIBRARIES})
    target_include_directories(vkx PRIVATE ${X11_INCLUDE_DIRS})
elseif(WIN32)
    set_property(TARGET vkx PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    target_sources(vkx PUBLIC

        "./Window/Win32Window.cpp"

        "./Window/Win32DnD.cpp"

        "./Window/Win32ImGUI.cpp"

        "./Window/Win32Vulkan.cpp"
        )
    target_link_libraries(vkx PRIVATE "user32")
endif()

target_include_directories(vkx 
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/ThirdParty/VMA/
    ${PROJECT_SOURCE_DIR}/Intermediate/Engine
    "./"
    )
    
target_link_libraries(vkx
    PRIVATE
    core
    ${Vulkan_LIBRARY}
    VulkanMemoryAllocator
    imgui
    imguizmo
    flecs
    assetlib
    portable_file_dialogs
    )


# Metacompiler

add_custom_target(vkx_metacompiler_run
    COMMAND metacompiler
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Run pre-compile step"
    SOURCES ${HEADERS})

add_dependencies(vkx vkx_metacompiler_run)