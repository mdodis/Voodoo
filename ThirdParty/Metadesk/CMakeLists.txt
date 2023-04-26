add_library(metadesk STATIC "compile.c")

target_include_directories(metadesk PUBLIC ".." PRIVATE ".")