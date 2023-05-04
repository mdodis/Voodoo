add_library(SPIRVReflect
    STATIC
    "./spirv_reflect.h"
    "./spirv_reflect.c")

target_include_directories(SPIRVReflect
    PUBLIC
    "./include"
    ".")