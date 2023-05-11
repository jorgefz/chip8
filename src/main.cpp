
#include "chip8/chip8.h"
#include <cassert>


int main(int argc, const char* argv[]) {

    auto chip8 = CHIP8::Interpreter();
    chip8.load_file("../test_games/snake.ch8");
    chip8.run();

}