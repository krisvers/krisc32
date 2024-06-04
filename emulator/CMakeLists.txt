cmake_minimum_required(VERSION 3.12)
project(k32-emu)

set(CMAKE_C_STANDARD 99)
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(k32-emu ${SOURCES})