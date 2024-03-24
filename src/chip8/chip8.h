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
#include <random>
#include <ctime>

#include <SFML/Graphics.hpp>
#include "state.h"
#include "renderer.h"

namespace CHIP8 {
    
    class Interpreter {
        State m_state;
        Renderer m_renderer;
        std::default_random_engine m_rng;
        double m_timer;
        double m_timer_freq; // Hz
    
    public:
        static constexpr int NATIVE_WIDTH  = 64;
        static constexpr int NATIVE_HEIGHT = 32;
        static constexpr int SCREEN_SCALE  = 16;
        static constexpr int SCREEN_WIDTH  = NATIVE_WIDTH  * SCREEN_SCALE;
        static constexpr int SCREEN_HEIGHT = NATIVE_HEIGHT * SCREEN_SCALE;

        Interpreter();

        /* Retrieve memory of virtual machine */
        State& get_state() { return m_state; }

        /* Loads a CHIP8 program into memory from disk */
        void load_file(std::string filename);

        /* Loads a CHIP8 program into memory from raw bytes*/
        void load_bytes(std::vector<byte_t> program);

        // load_state(filename)
        // save_state(filename)

        /* Executes the main loop and runs the loaded program */
        void run();

        /* Executes an opcode on the current state */
        void run_instruction(uint16_t code);

        /* Draws 8 monochrome pixels encoded as bits in a byte  */
        void draw_byte(byte_t x, byte_t y, byte_t byte);

        /*
        Draws a pixel in the canvas in white (0) or black (1).
        The pixel is XOR'd with the screen pixel at that location.
        If the location is outside the canvas, it wraps around.
        */
        void draw_pixel(byte_t x, byte_t y, bool pixel);

        /* Returns a random integer between 0 and 255 */
        byte_t random_byte();

        /* Advance Delay and Sound timers at a rate of 60 Hz (by default) */
        void update_timers(double dt);

    };

}

#endif /* CHIP8_H */
