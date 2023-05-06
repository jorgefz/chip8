#ifndef CHIP8_STATE_H
#define CHIP8_STATE_H

#include <array>
#include <unordered_map>
#include <cstdint>

namespace CHIP8 {
    
    typedef uint8_t byte_t;

    static constexpr uint16_t RAM_SIZE        = 0xFFF;
    static constexpr uint16_t RAM_PROG_OFFSET = 0x200;
    static constexpr byte_t   STACK_SIZE      = 16;
    static constexpr byte_t   REGISTER_NUM    = 16;

    struct State {
        // Memory
        std::array<byte_t,   RAM_SIZE>     ram;
        std::array<uint16_t, STACK_SIZE>   stack;
        std::array<byte_t,   REGISTER_NUM> regs;  // General purpose "V" registers

        // Pointers
        uint16_t pc;  // program counter
        byte_t   sp;  // stack pointer
        
        // Special registers
        byte_t   DTreg; // delay timer, 8bit register
        byte_t   STreg; // sound timer, 8bit register
        uint16_t Ireg;  // address, 16bit register

        // Methods
        void reset();
    };

}


#endif /* CHIP8_STATE_H */