#include "state.h"

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

        // Copy hex digits
        std::copy_n(HEX_DIGITS.begin(), HEX_ALPHABET_SIZE, ram.begin());
    }
}