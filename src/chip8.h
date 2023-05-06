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

#include <SFML/Graphics.hpp>
#include "state.h"

namespace CHIP8 {

    class Interpreter {
        State m_state;
        sf::Image m_canvas;
        sf::Texture m_texture;
        sf::Sprite m_sprite;
    public:
        static constexpr int NATIVE_WIDTH  = 64;
        static constexpr int NATIVE_HEIGHT = 32;
        static constexpr int SCREEN_SCALE  = 16;
        static constexpr int SCREEN_WIDTH  = NATIVE_WIDTH  * SCREEN_SCALE;
        static constexpr int SCREEN_HEIGHT = NATIVE_HEIGHT * SCREEN_SCALE;

        State* get_state();

        /* Loads a CHIP8 program into memory from disk */
        void load_file(std::string filename);

        /* Loads a CHIP8 program into memory from raw bytes*/
        void load_bytes(std::vector<byte_t> program);

        // load_state(filename)
        // save_state(filename)

        /* Executes the main loop and runs the loaded program */
        void run();

        /* Draws pixels encoded as bits */
        void draw_byte(byte_t x, byte_t y, byte_t byte);

        /*
        Draws a pixel in the canvas in white (0) or black (1).
        The pixel is XOR'd with the screen pixel at that location.
        If the location is outside the canvas, it wraps around.
        */
        void draw_pixel(byte_t x, byte_t y, bool white);

        /* Executes an opcode on the current state */
        void run_instruction(uint16_t code);
    };

}

#endif /* CHIP8_H */
