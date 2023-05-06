


#include "chip8.h"


int main(int argc, const char* argv[]) {

    auto chip8 = CHIP8::Interpreter();

    chip8.run();

    /*
    auto inter = CHIP8::Interpreter();
    inter.get_state()->reset();

    // Starting from 0x200
    inter.load_bytes({0x0, 0x0, 0x0, 0x0, 0x0, 0x00});

    while(inter.get_state()->pc < CHIP8::RAM_SIZE){
        inter.get_state()->pc += 2;
        uint8_t* ram = inter.get_state()->ram.data() + inter.get_state()->pc;
        uint16_t code = (ram[0] << 8) | ram[1];
        std::cout << std::hex <<"0x"<< inter.get_state()->pc << " [0x" << code << "]" << std::endl;
        inter.run_instruction(code);
    }
    */

}