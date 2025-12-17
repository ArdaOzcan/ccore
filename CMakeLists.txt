cmake_minimum_required(VERSION 3.10)
project(CCore C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)
set(C_STANDARD_REQUIRED ON)

add_compile_options(-Werror -g)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ccore STATIC ccore.c)
target_include_directories(ccore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_EXAMPLES "Build example executables" ON)
if(BUILD_EXAMPLES)
    add_executable(example_main example/main.c)
    add_executable(example_pool example/pool.c)
    target_compile_definitions(ccore PRIVATE CCORE_VERBOSE=1)
    target_link_libraries(example_main PRIVATE ccore)
    target_link_libraries(example_pool PRIVATE ccore)
endif()

