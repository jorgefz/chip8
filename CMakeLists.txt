cmake_minimum_required(VERSION 3.22.1)
project(chip8)

file (GLOB_RECURSE CHIP8_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${CHIP8_SOURCES})
