file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

add_library(ECS
    STATIC
    ${HEADERS}
    ${SOURCES}
    )

target_include_directories(ECS
    PUBLIC
    "."
    ${PROJECT_SOURCE_DIR}/Intermediate/Source/
    ${PROJECT_SOURCE_DIR}/Intermediate/Source/ECS
    PRIVATE
    "..")

target_link_libraries(ECS
    PUBLIC
    core
    flecs
    MokLib
    Renderer)

add_custom_target(ECSMetacompilerStep
    COMMAND metacompiler
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Run pre-compile step"
    SOURCES ${HEADERS})

add_dependencies(ECS ECSMetacompilerStep)