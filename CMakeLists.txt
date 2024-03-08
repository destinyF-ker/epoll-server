cmake_minimum_required(VERSION 3.10)
project(squash_server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES src/*.c)
# foreach(SOURCE IN LISTS SOURCES)
#     message("SOURCE: ${SOURCE}")
# endforeach()
add_executable(squash_server ${SOURCES})
target_include_directories(squash_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)