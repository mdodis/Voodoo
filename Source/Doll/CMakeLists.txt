file(GLOB SOURCES "*.cpp" "*.h")
add_executable(Doll ${SOURCES})

target_include_directories(Doll PRIVATE "./")

target_link_libraries(Doll PRIVATE MokLib metadesk)