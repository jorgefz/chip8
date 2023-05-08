
#include "chip8.h"
#include <cassert>

/*
Checks the initial state of the program
after being reset (ram, stack, and registers).
ALl should be zeroed except for RAM, which should also
store hex digits at 0x0 - 0x050.
*/
void test_state_reset(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();
    
    // Stack
    for(CHIP8::byte_t b : state.stack){
        if(b != 0x0) throw std::runtime_error("Stack byte not initialised to 0");
    }

    // Registers
    for(CHIP8::byte_t b : state.regs){
        if(b != 0x0) throw std::runtime_error("Register V not initialised to 0");
    }
    if(state.Ireg != 0x0)  throw std::runtime_error("Register I not initialised to 0");
    if(state.DTreg != 0x0) throw std::runtime_error("Register DT not initialised to 0");
    if(state.STreg != 0x0) throw std::runtime_error("Register ST not initialised to 0");

    // RAM
    /// TODO: check for hex digits stored from 0x000 to 0x050
    for(CHIP8::byte_t b : state.ram){
        if(b != 0x0) throw std::runtime_error("RAM byte not initialised to 0");
    }
}

/*
0x00EE - RET
The interpreter sets the program counter to
the address at the top of the stack,
then subtracts 1 from the stack pointer.
Should throw an error if the stack pointer
is at the bottom of the stack.
*/
void test_instr_RET(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    // Simulate being in subroutine
    state.sp = 1;
    state.stack[0] = 0xab;
    prog.run_instruction(0x00EE);
    if(state.pc != 0xab) throw std::runtime_error("Program counter not updated on RET");
    if(state.sp != 0) throw std::runtime_error("Stack pointer not updated on RET");

    // Ensure it fails when program is not in subroutine
    state.reset();
    state.sp = 0;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x00EE);
        exception_thrown = false;
    } catch (const std::exception& e) { }
    if(!exception_thrown) throw std::runtime_error("Exception not thrown when RET outside subroutine");
}

/*
0x1nnn - JMP nnn
The interpreter sets the program counter to nnn.

No need for bounds checking because 0xNNN can only store
values between 0x000 and 0xFFF, all of which are valid RAM addresses.
*/
void test_instr_JMP(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    prog.run_instruction(0x1123);
    if(state.pc != 0x123) throw std::runtime_error("Program counter not moved on JMP");

    // Beginning of ram
    prog.run_instruction(0x1000);
    if(state.pc != 0x000) throw std::runtime_error("Program counter not moved on JMP");

    // End of ram
    prog.run_instruction(0x1FFF);
    if(state.pc != 0xFFF) throw std::runtime_error("Program counter not moved on JMP");
}

/*
0x2nnn - CALL nnn
The interpreter increments the stack pointer,
then puts the current PC on the top of the stack.
The PC is then set to nnn.
*/
void test_instr_CALL(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.pc = 0x123;
    prog.run_instruction(0x2FFF);
    if(state.pc != 0xFFF) throw std::runtime_error("Program counter not updated on CALL");
    if(state.sp != 1) throw std::runtime_error("Stack pointer not updated on CALL");
    if(state.stack[0] != 0x123) throw std::runtime_error("Stack not updated on CALL");
    
    // Program fails when stack is full
    state.sp = CHIP8::STACK_SIZE;
    bool exception_thrown = true;
    try {
       prog.run_instruction(0x2FFF);
       exception_thrown = false;
    } catch (std::exception& e) { }
    if(!exception_thrown) throw std::runtime_error("Exception not thrown on stack overflow");
}


int main(int argc, const char* argv[]) {

    // test_state_reset();
    test_instr_RET();
    test_instr_JMP();
    test_instr_CALL();
    

    // auto chip8 = CHIP8::Interpreter();
    // chip8.run();

    /*
    auto inter = CHIP8::Interpreter();
    inter.get_state().reset();

    // Starting from 0x200
    inter.load_bytes({0x0, 0x0, 0x0, 0x0, 0x0, 0x00});

    while(inter.get_state().pc < CHIP8::RAM_SIZE){
        inter.get_state().pc += 2;
        uint8_t* ram = inter.get_state().ram.data() + inter.get_state().pc;
        uint16_t code = (ram[0] << 8) | ram[1];
        std::cout << std::hex <<"0x"<< inter.get_state().pc << " [0x" << code << "]" << std::endl;
        inter.run_instruction(code);
    }
    */

}