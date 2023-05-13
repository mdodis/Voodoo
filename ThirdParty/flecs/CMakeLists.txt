add_library(flecs STATIC
    "./flecs.h"
    "./flecs.c"
    )

set_target_properties(flecs PROPERTIES C_STANDARD 99)

if (WIN32)
    target_compile_definitions(flecs
        PRIVATE
        "_CRT_SECURE_NO_WARNINGS"
        )
endif()

target_include_directories(flecs PUBLIC ".")
target_compile_definitions(flecs PUBLIC FLECS_STATIC)
