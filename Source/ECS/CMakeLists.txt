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
    PRIVATE
    "..")

target_link_libraries(ECS
    PUBLIC
    core
    flecs
    MokLib
    Renderer)

# set(MODULE_FILES_PATH "${PROJECT_SOURCE_DIR}/Intermediate/Source/ModuleFiles.txt")

# add_custom_command(
#     OUTPUT ${MODULE_FILES_PATH}
#     COMMAND metacompiler
#     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#     COMMENT "Run pre-compile step"
#     DEPENDS ${HEADERS}
# )

# add_custom_target(ECSMetacompilerStep
#     DEPENDS ${MODULE_FILES_PATH}
# )

# add_dependencies(ECS ECSMetacompilerStep)
