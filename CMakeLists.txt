cmake_minimum_required(VERSION 3.10)
project(CCore C)

set(CMAKE_C_STANDARD 99)

add_library(ccore STATIC ccore.c)
target_include_directories(ccore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_EXAMPLES "Build example executables" ON)
if(BUILD_EXAMPLES)
    add_executable(ccore_example example/main.c)
    target_compile_definitions(ccore PRIVATE CCORE_VERBOSE=1)
    target_link_libraries(example PRIVATE ccore)
endif()

add_compile_options(-Werror -Wall)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

