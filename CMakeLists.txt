cmake_minimum_required(VERSION 3.5)

project(soft-renderer-2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(soft-renderer-2
    src/Main.cpp
    src/Rasterizer.hpp
    src/Rasterizer.cpp
)
