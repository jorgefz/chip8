
#include "chip8/chip8.h"
#include <cassert>


void program_analyser(const std::string& filename, int max){
    auto chip8 = CHIP8::Interpreter();
    chip8.load_file(filename);
    auto& ram = chip8.get_state().ram;
}

int main(int argc, const char* argv[]) {

    if(argc != 2){
        std::cout << 
        "Supply filename" << std::endl;
        return 1;
    }
    program_analyser(argv[1], 10);

    auto chip8 = CHIP8::Interpreter();
    chip8.load_file(argv[1]);
    chip8.run();

}