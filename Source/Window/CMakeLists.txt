file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

# Exclude ImGui source @todo: move this to editor mod somehow
list(FILTER HEADERS EXCLUDE REGEX ".*ImGUI.*\\.h")
list(FILTER SOURCES EXCLUDE REGEX ".*ImGUI.*\\.cpp")

# Exclude platform specific source files
if (WIN32)
    list(FILTER HEADERS EXCLUDE REGEX ".*GLFW.*\\.h")
    list(FILTER SOURCES EXCLUDE REGEX ".*GLFW.*\\.cpp")
elseif (UNIX)
    list(FILTER HEADERS EXCLUDE REGEX ".*Win32.*\\.h")
    list(FILTER SOURCES EXCLUDE REGEX ".*Win32.*\\.cpp")
endif()


add_library(Window STATIC ${SOURCES} ${HEADERS})

target_link_libraries(Window PUBLIC VulkanCommon)

# Link to platform specific libraries
if (WIN32)
    target_link_libraries(Window PRIVATE "user32")
elseif (UNIX)
    find_package(X11 REQUIRED)

    target_link_libraries(Window PRIVATE glfw ${X11_LIBRARIES})
    target_include_directories(Window PRIVATE ${X11_INCLUDE_DIRS})
endif()
