cmake_minimum_required(VERSION 3.12)
project(k32-emu)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

set(CMAKE_C_STANDARD 99)
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(k32-emu ${SOURCES})

target_include_directories(k32-emu PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(k32-emu PRIVATE SDL2::SDL2 ${SDL2_LIBRARIES})

if (MSVC)
	set_property(TARGET k32-emu PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
endif()
