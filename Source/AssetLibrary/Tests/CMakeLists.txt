
set(SOURCES
    "./AssetLibrary.test.cpp"
    "./Tests.cpp"
    )

add_executable(AssetLibraryTests ${SOURCES})

target_link_libraries(AssetLibraryTests PRIVATE
    AssetLibrary)

target_include_directories(AssetLibraryTests PRIVATE
    "../")

add_test(NAME AssetLibraryTestsTest COMMAND AssetLibraryTests)
set_property(TEST AssetLibraryTestsTest PROPERTY PASS_REGULAR_EXPRESSION "Passed")
set_property(TEST AssetLibraryTestsTest PROPERTY FAIL_REGULAR_EXPRESSION "Failed")
