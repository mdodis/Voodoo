set(SOURCES
    "./BlockList.h"
    "./DeletionQueue.h"
    "./Core.cpp"
)

add_library(core STATIC ${SOURCES})

target_link_libraries(core
    PUBLIC
    glm
    MokLib)

target_include_directories(core PRIVATE "./")

add_subdirectory(Tests)