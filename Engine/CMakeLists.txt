find_package(Vulkan)

add_executable(vkx
    "./Main.cpp"
    "./Window.cpp"
    "./Window.h"
    "./App.cpp"
    "./App.h"
    "./Engine.h"
    "./Engine.cpp"
    "./VulkanCommon.h"
     "VulkanCommon.cpp")

target_include_directories(vkx 
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    )

target_link_libraries(vkx
    PUBLIC
    ${Vulkan_LIBRARY}
    "user32"
    MokLib
    )

add_custom_command(TARGET vkx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo
    "Installing dll to output dir..."

    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:MokLib>
    $<TARGET_FILE_DIR:vkx>
    )