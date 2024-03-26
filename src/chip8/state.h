#ifndef CHIP8_STATE_H
#define CHIP8_STATE_H

#include <array>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

namespace CHIP8 {
    
    typedef uint8_t byte_t;

    static constexpr uint16_t RAM_SIZE        = 0x1000;
    static constexpr uint16_t RAM_PROG_OFFSET = 0x200;
    static constexpr byte_t   STACK_SIZE      = 16;
    static constexpr byte_t   REGISTER_NUM    = 16;

    // Number of bytes to store representation of one hex digit.
    static constexpr byte_t HEX_DIGIT_SIZE = 5;
    // Number of bytes to store representation of all hex digits ('0' to 'F')
    static constexpr byte_t HEX_ALPHABET_SIZE = 0x10 * HEX_DIGIT_SIZE;
    
    static const std::array<byte_t, HEX_ALPHABET_SIZE> HEX_DIGITS = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
        0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
        0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
        0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
        0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
        0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
        0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
        0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
        0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
        0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
        0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
        0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
        0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
        0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
        0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
        0xF0, 0x80, 0xF0, 0x80, 0x80, /* F */
    };

    struct State {
        // Memory
        std::array<byte_t,   RAM_SIZE>     ram;
        std::array<uint16_t, STACK_SIZE>   stack;

        // Registers
        std::array<byte_t,   REGISTER_NUM> regs;
        byte_t   DTreg; // delay timer, 8bit register
        byte_t   STreg; // sound timer, 8bit register
        uint16_t Ireg;  // address store, 16bit register

        // Pointers
        uint16_t pc;  // Program counter
                      // Index of RAM where current instruction is being run.
        byte_t   sp;  // Stack pointer.
                      // Treated as current stack size.
                      // Access top of stack with `stack[sp-1]`.

        // Methods

        /* Resets all memory */
        void reset();

        /* Moves the program counter and returns the last opcode */
        uint16_t advance();
    };

}


#endif /* CHIP8_STATE_H */