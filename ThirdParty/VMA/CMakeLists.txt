find_package(Vulkan REQUIRED)

add_library(VulkanMemoryAllocator STATIC
    "./vk_mem_alloc.h"
    "./vk_mem_alloc.cpp"
)

target_include_directories(VulkanMemoryAllocator PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    "./"
)

target_link_libraries(VulkanMemoryAllocator PUBLIC
    ${Vulkan_LIBRARY}
)