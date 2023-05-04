file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(Renderer STATIC ${SOURCES} ${HEADERS})

target_include_directories(Renderer
    PUBLIC
    "${PROJECT_SOURCE_DIR}/ThirdParty/VMA/")

target_link_libraries(Renderer
    PUBLIC
    core
    AssetLibrary
    VulkanCommon
    PRIVATE
    VulkanMemoryAllocator
    SPIRVReflect)