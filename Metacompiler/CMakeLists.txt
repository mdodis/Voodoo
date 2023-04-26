file(GLOB SOURCES "*.cpp" "*.h")

add_executable(metacompiler ${SOURCES})

target_include_directories(metacompiler
    PUBLIC "./")

target_link_libraries(metacompiler
    PRIVATE
    MokLib
    metadesk)