
set(SOURCES
    "./AssetLibrary.test.cpp"
    "./Tests.cpp"
    )

add_executable(assetlib_tests ${SOURCES})

target_link_libraries(assetlib_tests PRIVATE
    assetlib)

target_include_directories(assetlib_tests PRIVATE
    "../")

add_test(NAME AssetLibraryTests COMMAND assetlib_tests)
set_property(TEST AssetLibraryTests PROPERTY PASS_REGULAR_EXPRESSION "Passed")
set_property(TEST AssetLibraryTests PROPERTY FAIL_REGULAR_EXPRESSION "Failed")
