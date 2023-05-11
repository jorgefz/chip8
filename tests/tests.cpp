
#include "../src/chip8/chip8.h"
#include <catch2/catch_test_macros.hpp>

/*
Checks the initial state of the program
after being reset (ram, stack, and registers).
ALl should be zeroed except for RAM, which should also
store hex digits at 0x0 - 0x050.
*/
TEST_CASE( "Check the initial state of the virtual machine", "[state]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();

    // Program counter must be at program section of RAM
    REQUIRE(state.pc == 0x200);

    // Stack pointer at bottom of stack
    REQUIRE(state.sp == 0); 

    // Stack must be zeroed
    for(CHIP8::byte_t b : state.stack){
        REQUIRE(b == 0x0);
    }

    // Registers must be zeroed
    for(CHIP8::byte_t b : state.regs){
        REQUIRE(b == 0x0);
    }
    REQUIRE(state.Ireg  == 0x0);
    REQUIRE(state.DTreg == 0x0);
    REQUIRE(state.STreg == 0x0);

    // RAM must be zeroed from program section
    int ram_zeroed = 0;
    for(uint16_t i = CHIP8::RAM_PROG_OFFSET; i != CHIP8::RAM_SIZE; ++i){
        ram_zeroed |= state.ram[i];
    }
    REQUIRE(ram_zeroed == 0);

    /// TODO: check for hex digits stored on RAM from 0x000 to 0x050
}

/*
00E0 - CLS
Clear the display.
*/
TEST_CASE("Clear display instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    prog.get_canvas().create(
        CHIP8::Interpreter::NATIVE_WIDTH,
        CHIP8::Interpreter::NATIVE_HEIGHT,
        sf::Color::White
    );
    prog.run_instruction(0x00E0);

    int screen_zeroed = 0;
    for(int x = 0; x != CHIP8::Interpreter::NATIVE_WIDTH; ++x){
        for(int y = 0; y != CHIP8::Interpreter::NATIVE_HEIGHT; ++y){
            screen_zeroed |= !(prog.get_canvas().getPixel(x, y) == sf::Color::Black);
        }
    }
    REQUIRE(screen_zeroed == 0);
}

/*
00EE - RET
The interpreter sets the program counter to
the address at the top of the stack,
then subtracts 1 from the stack pointer.
Should throw an error if the stack pointer
is at the bottom of the stack.
*/
TEST_CASE("Subroutine return instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    // Simulate being in subroutine
    state.sp = 1;
    state.stack[0] = 0xab;
    prog.run_instruction(0x00EE);

    REQUIRE(state.pc == 0xab);
    REQUIRE(state.sp == 0);

    // Ensure it fails when program is not in subroutine
    state.reset();
    state.sp = 0;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x00EE);
        exception_thrown = false;
    } catch (const std::exception& e) { }
    REQUIRE(exception_thrown);
}

/*
1nnn - JMP nnn
The interpreter sets the program counter to nnn.
No need for bounds checking because 0xNNN can only store
values between 0x000 and 0xFFF, all of which are valid RAM addresses.
*/
TEST_CASE("Jump instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    prog.run_instruction(0x1123);
    REQUIRE(state.pc == 0x123);
    
    // Beginning of ram
    prog.run_instruction(0x1000);
    REQUIRE(state.pc == 0);

    // End of ram
    prog.run_instruction(0x1FFF);
    REQUIRE(state.pc == 0xFFF);
}

/*
2nnn - CALL nnn
The interpreter increments the stack pointer,
then puts the current PC on the top of the stack.
The PC is then set to nnn.
*/
TEST_CASE("Subroutine call instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.pc = 0x123;
    prog.run_instruction(0x2FFF);

    REQUIRE(state.pc == 0xFFF);
    REQUIRE(state.sp == 0x1);
    REQUIRE(state.stack[0] == 0x123);
    
    // Program fails when stack is full
    state.sp = CHIP8::STACK_SIZE;
    bool exception_thrown = true;
    try {
       prog.run_instruction(0x2FFF);
       exception_thrown = false;
    } catch (std::exception& e) { }
    REQUIRE(exception_thrown);
}

/*
3xkk - SE Vx, kk
The interpreter compares register Vx to kk,
and if they are equal, increments the program counter by 2.
Here, we need to check for RAM overflow.
*/
TEST_CASE("Jump if register equals byte instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xbc;
    prog.run_instruction(0x3abc);
    REQUIRE(state.pc == 0x2);
    
    state.pc = 0;
    prog.run_instruction(0x3abc + 1);
    REQUIRE(state.pc == 0x0);

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x3000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    REQUIRE((exception_thrown && state.pc <= 0xFFF));
}

/*
4xkk - SNE Vx, byte
The interpreter compares register Vx to kk, and if they are not equal,
increments the program counter by 2.
*/
TEST_CASE("Jump if register does not equal byte instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xbc;
    prog.run_instruction(0x4abc);
    REQUIRE(state.pc == 0);

    state.pc = 0;
    prog.run_instruction(0x4abc + 1);
    REQUIRE(state.pc == 0x2);

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x3000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    REQUIRE((exception_thrown && state.pc <= 0xFFF));
}


/*
5xy0 - SE Vx, Vy
Skip next instruction if Vx = Vy.
The interpreter compares register Vx to register Vy,
and if they are equal, increments the program counter by 2.
*/
TEST_CASE("Jump if registers equal instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xcc;
    state.regs[0xb] = 0xcc;
    prog.run_instruction(0x5ab0);
    REQUIRE(state.pc == 0x2);

    state.pc = 0;
    state.regs[0xa] = 0x00;
    state.regs[0xb] = 0x01;
    prog.run_instruction(0x5ab0);
    REQUIRE(state.pc == 0x0);
    
    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    state.regs[0] = 0;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x5000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    REQUIRE((exception_thrown && state.pc <= 0xFFF));
}


/*
6xkk - LD Vx, byte
The interpreter puts the value kk into register Vx.
*/
TEST_CASE("Set register value instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    prog.run_instruction(0x6abb);
    REQUIRE(state.regs[0xa] == 0xbb);
}

/*
7xkk - ADD Vx, byte
Set Vx = Vx + kk.
Adds the value kk to the value of register Vx, then stores the result in Vx. 
*/
/// TODO: check if carry flag VF should be set on overflow.
TEST_CASE("Adds byte to register instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    prog.run_instruction(0x7abb);
    REQUIRE(state.regs[0xa] == 0xbb);

    // Register overflow
    state.regs[0xb] = 0x1;
    prog.run_instruction(0x7bff);
    REQUIRE(state.regs[0xb] == 0x0);
}

/*
8xy0 - LD Vx, Vy
Set Vx = Vy.
Stores the value of register Vy in register Vx.
*/
TEST_CASE("Copies register instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xb] = 0xff;
    state.regs[0xa] = 0x00;
    prog.run_instruction(0x8ab0);
    REQUIRE(state.regs[0xa] == 0xFF);
}

/*
8xy1 - OR Vx, Vy
Set Vx = Vx OR Vy.
Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx.
*/
TEST_CASE("Register bitwise 'or' instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab1);
    REQUIRE(state.regs[0xa] == (0xAA | 0xBB));
}

/*
8xy2 - AND Vx, Vy
Set Vx = Vx AND Vy.
Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx.
*/
TEST_CASE("Register bitwise 'and' instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab2);
    REQUIRE(state.regs[0xa] == (0xAA & 0xBB));
}

/*
8xy3 - XOR Vx, Vy
Set Vx = Vx XOR Vy.
Performs a bitwise exclusive OR on the values of Vx and Vy,
then stores the result in Vx.
*/
TEST_CASE("Register bitwise 'xor' instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab3);
    REQUIRE(state.regs[0xA] == (0xAA ^ 0xBB));
}


/*
8xy4 - ADD Vx, Vy
Set Vx = Vx + Vy, set VF = carry.
The values of Vx and Vy are added together.
If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
Only the lowest 8 bits of the result are kept, and stored in Vx.
*/
TEST_CASE("Add up registers instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    // No overflow
    state.regs[0xa] = 0x22;
    state.regs[0xb] = 0x33;
    prog.run_instruction(0x8ab4);
    REQUIRE(state.regs[0xa] == (0x22 + 0x33));
    REQUIRE(state.regs[0xf] == 0x0);

    // Overflow
    state.regs[0xa] = 0x01;
    state.regs[0xb] = 0xff;
    prog.run_instruction(0x8ab4);
    REQUIRE(state.regs[0xa] == 0x0);
    REQUIRE(state.regs[0xf] == 0x1);
}

/*
8xy5 - SUB Vx, Vy
Set Vx = Vx - Vy, set VF = NOT borrow.
If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
*/
TEST_CASE("Subtract registers instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0x22;
    prog.run_instruction(0x8ab5);

    REQUIRE(state.regs[0xa] == (0xAA - 0x22));
    REQUIRE(state.regs[0xF] == 0x1);

    // Register overflow
    state.regs[0xa] = 0x00;
    state.regs[0xb] = 0x01;
    prog.run_instruction(0x8ab5);
    
    REQUIRE(state.regs[0xa] == 0xFF);
    REQUIRE(state.regs[0xf] == 0x0);
}

/*
8xy6 - SHR Vx {, Vy}
Set Vx = Vx SHR 1.
If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0.
Then Vx is divided by 2 (right-shifted).
Originally, this right-shifted the value of Vy and stored it on Vx.
*/
TEST_CASE("Register bitwise right-shift instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0x10;
    prog.run_instruction(0x8a06);

    REQUIRE(state.regs[0xa] == (0x10 >> 1));
    REQUIRE(state.regs[0xF] == 0x0);

    state.regs[0xa] = 0x11;
    prog.run_instruction(0x8a06);

    REQUIRE(state.regs[0xa] == (0x11 >> 1));
    REQUIRE(state.regs[0xF] == 0x1);
}


/*
8xy7 - SUBN Vx, Vy
Set Vx = Vy - Vx, set VF = NOT borrow.
If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
*/
TEST_CASE("Subtract registers and reverse sign instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab7);
    REQUIRE(state.regs[0xa] == (0xBB - 0xAA));
    REQUIRE(state.regs[0xF] == 0x1);

    // Register overflow
    state.regs[0xa] = 0x01;
    state.regs[0xb] = 0x00;
    prog.run_instruction(0x8ab7);
    REQUIRE(state.regs[0xa] == 0xff);
    REQUIRE(state.regs[0xF] == 0x0);
}


/*
8xyE - SHL Vx {, Vy}
Set Vx = Vx SHL 1.
If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
Then Vx is multiplied by 2 (left-shifted).
Originally, this left-shifted the value of Vy and stored it on Vx.
*/
TEST_CASE("Register bitwise left-shift instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0x01;
    prog.run_instruction(0x8a0E);
    REQUIRE(state.regs[0xa] == (0x01 << 1));
    REQUIRE(state.regs[0xF] == 0x0);

    state.regs[0xa] = 0x80;
    prog.run_instruction(0x8a0E);
    REQUIRE(state.regs[0xa] == CHIP8::byte_t(0x80 << 1));
    REQUIRE(state.regs[0xF] == 0x1);
}


/*
9xy0 - SNE Vx, Vy
Skip next instruction if Vx != Vy.
The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
*/
TEST_CASE("Jump if registers do not equal instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xff;
    state.regs[0xb] = 0xff;
    prog.run_instruction(0x9ab0);
    REQUIRE(state.pc == 0);

    state.pc = 0;
    state.regs[0xa] = 0xff;
    state.regs[0xb] = 0x00;
    prog.run_instruction(0x9ab0);
    REQUIRE(state.pc == 0x2);

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    state.regs[0xa] = 0xff;
    state.regs[0xb] = 0x00;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x9ab0);
        exception_thrown = false;
    } catch (std::exception& e) { }
    REQUIRE((exception_thrown && state.pc <= 0xFFF));
}

/*
Annn - LD I, addr
Set I = nnn.
The value of register I is set to nnn.
Cannot overflow because tribble cannot store a value greater than RAM size.
*/
TEST_CASE("Set I address register instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    prog.run_instruction(0xA123);
    REQUIRE(state.Ireg == 0x123);
}

/*
Bnnn - JP V0, addr
Jump to location nnn + V0.
The program counter is set to nnn plus the value of V0.
*/
TEST_CASE("Jump to V0 plus offset instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.pc = 0;
    state.regs[0x0] = 0x0;
    prog.run_instruction(0xB123);
    REQUIRE(state.pc == 0x123);

    state.pc = 0x0;
    state.regs[0x0] = 0xAA;
    prog.run_instruction(0xB0BB);
    REQUIRE(state.pc == (0xAA + 0xBB));

    // RAM overflow
    state.regs[0x0] = 0xFF;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0xBFFF);
        exception_thrown = false;
    } catch (std::exception& e){ }
    REQUIRE(exception_thrown);
}

/*
Cxkk - RND Vx, byte
Set Vx = random byte AND kk.
The interpreter generates a random number from 0 to 255,
which is then ANDed with the value kk. The results are stored in Vx.
*/
TEST_CASE("Random byte instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;
    /// TODO: need to reset rng between calls to test
}

/*
Dxyn - DRW Vx, Vy, nibble
Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
The interpreter reads n bytes from memory, starting at the address stored in I.
These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
Sprites are XORed onto the existing screen.
If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
If the sprite is positioned so part of it is outside the coordinates of the display,
it wraps around to the opposite side of the screen. 
*/
/// TODO

/*
Ex9E - SKP Vx
Skip next instruction if key with the value of Vx is pressed.
Checks the keyboard, and if the key corresponding
to the value of Vx is currently in the down position, PC is increased by 2.
*/
/// TODO

/*
ExA1 - SKNP Vx
Skip next instruction if key with the value of Vx is not pressed.
Checks the keyboard, and if the key corresponding
to the value of Vx is currently in the up position, PC is increased by 2.
*/
/// TODO

/*
Fx07 - LD Vx, DT
Set Vx = delay timer value.
The value of DT is placed into Vx.
*/
TEST_CASE("Get DT register value instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.DTreg = 0xaa;
    prog.run_instruction(0xFa07);
    REQUIRE(state.regs[0xa] == 0xAA);
}

/*
Fx0A - LD Vx, K
Wait for a key press, store the value of the key in Vx.
All execution stops until a key is pressed, then the value of that key is stored in Vx.
*/
/// TODO


/*
Fx15 - LD DT, Vx
Set delay timer = Vx.
DT is set equal to the value of Vx.
*/
TEST_CASE("Set DT register instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    prog.run_instruction(0xFa15);
    REQUIRE(state.DTreg == 0xAA);
}

/*
Fx18 - LD ST, Vx
Set sound timer = Vx.
ST is set equal to the value of Vx.
*/
TEST_CASE("Set ST register instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    prog.run_instruction(0xFa18);
    REQUIRE(state.STreg == 0xAA);
}

/*
Fx1E - ADD I, Vx
Set I = I + Vx.
The values of I and Vx are added, and the results are stored in I.
*/
TEST_CASE("Add to register I instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xaa;
    state.Ireg = 0x1;
    prog.run_instruction(0xFa1E);
    REQUIRE(state.Ireg == (0xAA + 0x1));
}

/*
Fx29 - LD F, Vx
Set I = location of sprite for digit Vx.
The value of I is set to the location for the hexadecimal sprite
corresponding to the value of Vx.
*/
TEST_CASE("Get location of hex digit sprite instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 0xF; // get sprite for F
    prog.run_instruction(0xFa29);
    REQUIRE(state.Ireg == (0xF * 5));
}

/*
Fx33 - LD B, Vx
Store BCD representation of Vx in memory locations I, I+1, and I+2.
The interpreter takes the decimal value of Vx,
and places the hundreds digit in memory at location in I,
the tens digit at location I+1
and the ones digit at location I+2.
*/
TEST_CASE("Get decimal representation of register value instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;

    state.regs[0xa] = 123;
    prog.run_instruction(0xFa33);
    REQUIRE((
        state.ram[state.Ireg]   == 1 &&
        state.ram[state.Ireg+1] == 2 &&
        state.ram[state.Ireg+2] == 3
    ));
}


/*
Fx55 - LD [I], Vx
Store registers V0 through Vx in memory starting at location I.
The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
*/
TEST_CASE("Copy V register values to RAM instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;
    state.Ireg = 0x80;

    CHIP8::byte_t x = 0xa; // store V0 to Va
    for(CHIP8::byte_t i = 0x0; i != x + 1; ++i){
        state.regs[i] = i;
    }

    prog.run_instruction(0xFa55);
    
    for(CHIP8::byte_t i = 0x0; i != x + 1; ++i){
        REQUIRE(state.ram[state.Ireg + i] == state.regs[i]);
    }
}

/*
Fx65 - LD Vx, [I]
Read registers V0 through Vx from memory starting at location I.
The interpreter reads values from memory starting at location I into registers V0 through Vx.
*/
TEST_CASE("Set V register values from RAM instruction", "[opcodes]"){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.pc = 0;
    state.Ireg = 0x80;

    CHIP8::byte_t x = 0xa; // store V0 to Va
    for(CHIP8::byte_t i = 0x0; i != x + 1; ++i){
        state.ram[state.Ireg + i] = i;
    }

    prog.run_instruction(0xFa65);
    
    for(CHIP8::byte_t i = 0x0; i != x + 1; ++i){
        REQUIRE(state.ram[state.Ireg + i] == state.regs[i]);
    }
}

