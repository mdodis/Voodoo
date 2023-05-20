file(GLOB SOURCES "*.cpp")

add_executable(Standalone ${SOURCES})

target_link_libraries(Standalone
    PRIVATE
    Engine
    Renderer
    ECS
    Module_Builtin)