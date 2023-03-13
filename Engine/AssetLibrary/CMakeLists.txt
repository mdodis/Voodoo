file(GLOB SOURCES "*.cpp" "*.h")

add_library(assetlib STATIC ${SOURCES})

target_include_directories(assetlib PRIVATE "./")

target_link_libraries(assetlib PUBLIC core)

add_subdirectory(Tests)