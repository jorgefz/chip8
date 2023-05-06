/*
--- TO-DO ---
1) Wrap state in struct.
2) Separate class Interpreter.
3) Use SFML for graphics. Install directly on Linux.

Each step:
1) Increase program counter
2) Read instruction
3) Run instruction
4) Update sound timer
5) Render window
*/

#ifndef CHIP8_H
#define CHIP8_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "state.h"

namespace CHIP8 {

    class Interpreter {
        State m_state;
    public:
        State* get_state();
        void load_file(std::string filename);
        void load_bytes(std::vector<byte_t> program);
        // load_state(filename)
        // save_state(filename)
        void run_instruction(uint16_t code);
    };

}

#endif /* CHIP8_H */
