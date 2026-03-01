cmake_minimum_required(VERSION 3.16)
project(SimEngine)

set(CMAKE_CXX_STANDARD 17)

add_executable(sim_eng
    src/main.cpp
)

target_compile_options(sim_eng PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Wunused-result
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
)

target_include_directories(sim_eng PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event
)