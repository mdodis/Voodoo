
set(SOURCES
    "./AssetLibrary.test.cpp"
    "./Tests.cpp"
    )

add_executable(AssetLibraryTests ${SOURCES})

target_link_libraries(AssetLibraryTests PRIVATE
    AssetLibrary)

target_include_directories(AssetLibraryTests PRIVATE
    "../")

v_add_test(AssetLibraryTests)