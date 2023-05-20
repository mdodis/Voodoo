file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

# Exclude platform specific source files
if (WIN32)
    list(FILTER HEADERS EXCLUDE REGEX ".*GLFW.*\\.h")
    list(FILTER SOURCES EXCLUDE REGEX ".*GLFW.*\\.cpp")
elseif (UNIX)
    list(FILTER HEADERS EXCLUDE REGEX ".*Win32.*\\.h")
    list(FILTER SOURCES EXCLUDE REGEX ".*Win32.*\\.cpp")
endif()

add_executable(Editor ${SOURCES} ${HEADERS})

target_link_libraries(Editor
    PRIVATE
    Engine
    Renderer
    ECS
    imgui
    portable_file_dialogs
    VulkanCommon
    Module_Builtin)