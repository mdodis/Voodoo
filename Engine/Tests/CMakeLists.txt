
set(SOURCES
    "./BlockList.test.cpp"
    "./Tests.cpp"
    )

add_executable(EngineTests ${SOURCES})

target_link_libraries(EngineTests
    MokLib)

target_include_directories(EngineTests PRIVATE
    "../")