cmake_minimum_required(VERSION 3.16)

project(minbox-runtime LANGUAGES CXX)

add_executable(minbox-runtime
    src/main.cpp
)

target_compile_features(minbox-runtime PRIVATE cxx_std_17)

target_compile_options(minbox-runtime PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
