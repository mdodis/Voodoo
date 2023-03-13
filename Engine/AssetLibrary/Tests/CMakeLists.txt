
set(SOURCES
    "./AssetLibrary.test.cpp"
    "./Tests.cpp"
    )

add_executable(assetlib_tests ${SOURCES})

target_link_libraries(assetlib_tests PRIVATE
    assetlib)

target_include_directories(assetlib_tests PRIVATE
    "../")