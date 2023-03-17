add_library(LZ4 STATIC
    "./lz4.c"
    "./lz4.h")

target_include_directories(LZ4 PUBLIC "./")