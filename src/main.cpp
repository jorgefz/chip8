
#include "chip8/chip8.h"

int main(int argc, const char* argv[]) {

    if(argc != 2){
        std::cout << 
        "Supply filename" << std::endl;
        return 1;
    }
    
    auto chip8 = CHIP8::Interpreter();
    chip8.load_file(argv[1]);
    chip8.run();

}