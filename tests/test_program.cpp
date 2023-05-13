#include "../src/chip8/chip8.h"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("Ensure hex digits are drawn on screen", "[program]"){

    std::cout <<
        "Press keys 0-9 and A-F and make sure they display on screen"
    << std::endl;

    auto chip8 = CHIP8::Interpreter();
    std::vector<CHIP8::byte_t> data{
        0xF0,0x0A, // Halt execution until key press, and store in V0 (digit to display)
        // 0x61,0x0F, // Set V1 to x position
        // 0x62,0x0A, // Set V2 to y position
        0x81,0x00, // Set V1 to V0
        0x71,0x01, // Add 1 to V1
        0x82,0x00, // Set V2 to V0
        0x82,0x0E, // Double the value of V2
        0xF0,0x29, // Set I to location of sprite representing value of V0 
        0x00,0xE0, // Clear screen
        0xD1,0x25, // Display 5 bytes from location I at screen position V1,V2
        0x12,0x00  // Jump to 0x200
    };
    chip8.load_bytes(data);
    chip8.run();
}