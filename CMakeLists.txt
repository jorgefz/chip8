cmake_minimum_required(VERSION 3.22.1)

project(chip8)
file (GLOB_RECURSE CHIP8_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(chip8 ${CHIP8_SOURCES})
target_link_libraries(chip8 sfml-graphics sfml-audio sfml-window sfml-system)
