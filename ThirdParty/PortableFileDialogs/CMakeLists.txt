cmake_minimum_required(VERSION 3.1.0)


add_library(portable_file_dialogs INTERFACE)
target_include_directories(portable_file_dialogs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})