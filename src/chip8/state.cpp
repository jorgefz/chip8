#include "state.h"
#include <stdexcept>

namespace CHIP8 {
    
    void State::reset(){
        pc    = 0;
        sp    = 0;
        DTreg = 0;
        STreg = 0;
        Ireg  = 0;
        ram.fill(0);
        stack.fill(0);
        regs.fill(0);

        // Copy hex digits to the front of the RAM
        std::copy_n(HEX_DIGITS.begin(), HEX_ALPHABET_SIZE, ram.begin());
    }

    uint16_t State::advance(){
        if(pc + 2 >= RAM_SIZE){
            throw std::runtime_error("RAM overflow");
        }
        uint16_t code = (ram[pc] << 8) | ram[pc+1];
        pc += 2;
        return code;
    }

    /* Jumps to the specified address in RAM */
    void State::jump(uint16_t address){
        // Check RAM overflow and integer overflow
        if(pc + address >= CHIP8::RAM_SIZE || pc + address < pc){
            throw std::runtime_error("RAM overflow");
        }
        pc += address;
    }
}