add_library(flecs STATIC
    "./flecs.h"
    "./flecs.c"
    )

set_target_properties(flecs PROPERTIES C_STANDARD 99)

target_include_directories(flecs PUBLIC ".")
target_compile_definitions(flecs PUBLIC FLECS_STATIC)
