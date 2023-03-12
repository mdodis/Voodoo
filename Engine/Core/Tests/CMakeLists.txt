
set(SOURCES
    "./BlockList.test.cpp"
    "./Tests.cpp"
    )

add_executable(core_tests ${SOURCES})

target_link_libraries(core_tests PRIVATE
    core)

target_include_directories(core_tests PRIVATE
    "../")