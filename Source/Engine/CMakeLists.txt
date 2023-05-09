file(GLOB_RECURSE SOURCES "**.cpp")
file(GLOB_RECURSE HEADERS "*.h")

add_library(Engine STATIC ${SOURCES} ${HEADERS})
target_include_directories(Engine PRIVATE "." PUBLIC "..")

# Link Third-party libraries
target_link_libraries(Engine
    PUBLIC
    ECS
    SflUUID
    PRIVATE
    core
    AssetLibrary
    VulkanCommon
    Renderer
    Window)