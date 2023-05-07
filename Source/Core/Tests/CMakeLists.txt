
set(SOURCES
    "./BlockList.test.cpp"
    "./Archive.test.cpp"
    "./Handle.test.cpp"
    "./Tests.cpp"
    )

add_executable(core_tests ${SOURCES})

target_link_libraries(core_tests PRIVATE
    core)

target_include_directories(core_tests PRIVATE
    "../")

add_test(NAME CoreTests COMMAND core_tests)
set_property(TEST CoreTests PROPERTY PASS_REGULAR_EXPRESSION "Passed")
set_property(TEST CoreTests PROPERTY FAIL_REGULAR_EXPRESSION "Failed")
