cmake_minimum_required(VERSION 3.20)
project(C_shell_overwrite C)

set(CMAKE_C_STANDARD 99)

add_executable(C_shell_overwrite main.c)

#target_include_directories(untitled PUBLIC include/libexplain-1.4/libexplain)
#add_library(LibExplain Include/libexplain-1.4/libexplain/waitpid.c Include/libexplain-1.4/libexplain/waitpid.h)
#target_include_directories(LibExplain PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>)
