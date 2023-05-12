cmake_minimum_required(VERSION 3.22.1)

# Main interpreter project
project(chip8 LANGUAGES CXX VERSION 0.1.0)
file (GLOB_RECURSE CHIP8_SOURCES CONFIGURE_DEPENDS "src/chip8/*.cpp")
add_executable(chip8 src/main.cpp ${CHIP8_SOURCES})
target_link_libraries(chip8 PUBLIC sfml-graphics sfml-audio sfml-window sfml-system)
set(CMAKE_CXX_FLAGS "-ggdb -O0") # debugging

# Tests project
find_package(Catch2 3 REQUIRED)
add_executable(tests tests/tests.cpp src/chip8/chip8.cpp src/chip8/state.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PUBLIC sfml-graphics sfml-audio sfml-window sfml-system)
include(CTest)
include(Catch)
catch_discover_tests(tests)
