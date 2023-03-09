set(SOURCES
    "./BlockList.h"
    "./DeletionQueue.h"
    "./IntrusiveList.h"
    "./ECS/Component.h"
    "./ECS/Component.cpp"
    "./ECS/Entity.h"
    "./ECS/World.h"
    "./ECS/World.cpp"
)

add_library(core STATIC ${SOURCES})

target_link_libraries(core
    PUBLIC
    glm
    MokLib)

target_include_directories(core PRIVATE "./")

add_subdirectory(Tests)