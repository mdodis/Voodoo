file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(VulkanCommon STATIC ${SOURCES} ${HEADERS})

find_package(Vulkan REQUIRED)

target_include_directories(VulkanCommon
    PRIVATE
    "."
    PUBLIC
    ".."
    ${Vulkan_INCLUDE_DIRS})

target_link_libraries(VulkanCommon
    PUBLIC
    core
    ${Vulkan_LIBRARY})