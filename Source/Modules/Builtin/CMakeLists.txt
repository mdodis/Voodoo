file (GLOB_RECURSE SOURCES "*.cpp")
file (GLOB HEADERS "*.h")

add_library(Module_Builtin ${SOURCES} ${HEADERS})
target_include_directories(Module_Builtin
    PUBLIC
    "../"
    "Intermediate"
    PRIVATE
    "./"
    )

target_link_libraries(Module_Builtin
    PUBLIC
    Engine
    Renderer
    ECS
    MokLib)

set(MODULE_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/ModFiles.txt")
add_custom_command(
    OUTPUT ${MODULE_FILES_PATH}
    COMMAND $<TARGET_FILE:Doll> preprocess -path "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Run pre-compile step"
    DEPENDS ${HEADERS}
)

add_custom_target(BuiltinPreprocessStep
    DEPENDS ${MODULE_FILES_PATH}
)

add_dependencies(Module_Builtin BuiltinPreprocessStep)
