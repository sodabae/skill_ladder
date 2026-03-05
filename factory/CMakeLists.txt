cmake_minimum_required(VERSION 3.16)
project(factory)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(factory
    src/main.cpp
)

target_include_directories(factory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)