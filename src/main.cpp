
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
00EE - RET
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
1nnn - JMP nnn
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
2nnn - CALL nnn
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

/*
3xkk - SE Vx, kk
The interpreter compares register Vx to kk,
and if they are equal, increments the program counter by 2.
Here, we need to check for RAM overflow.
*/
void test_instr_SE(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xbc;
    prog.run_instruction(0x3abc);
    if(state.pc != 2) throw std::runtime_error("Program counter not updated when it should on SE");
    
    state.pc = 0;
    prog.run_instruction(0x3abc + 1);
    if(state.pc > 0) throw std::runtime_error("Program counter updated when it shouldn't on SE");

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x3000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    if(!exception_thrown || state.pc > 0xFFF){
        throw std::runtime_error("RAM overflow exception not thrown on SE");
    }
}

/*
4xkk - SNE Vx, byte
The interpreter compares register Vx to kk, and if they are not equal,
increments the program counter by 2.
*/
void test_instr_SNE(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xbc;
    prog.run_instruction(0x4abc);
    if(state.pc > 0) throw std::runtime_error("Program counter updated when it shouldn't on SNE");
    
    state.pc = 0;
    prog.run_instruction(0x4abc + 1);
    if(state.pc != 2) throw std::runtime_error("Program counter not updated when it should on SNE");

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x3000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    if(!exception_thrown || state.pc > 0xFFF){
        throw std::runtime_error("RAM overflow exception not thrown on SNE");
    }
}


/*
5xy0 - SE Vx, Vy
Skip next instruction if Vx = Vy.

The interpreter compares register Vx to register Vy,
and if they are equal, increments the program counter by 2.
*/
void test_instr_SE_regs(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xcc;
    state.regs[0xb] = 0xcc;
    prog.run_instruction(0x5ab0);
    if(state.pc != 2) throw std::runtime_error("Program counter not updated when it should on SE-regs");
    
    state.pc = 0;
    state.regs[0xa] = 0x00;
    state.regs[0xb] = 0x01;
    prog.run_instruction(0x5ab0);
    if(state.pc > 0) throw std::runtime_error("Program counter updated when it shouldn't on SE-regs");

    // RAM overflow
    state.reset();
    state.pc = 0xFFF;
    state.regs[0] = 0;
    bool exception_thrown = true;
    try {
        prog.run_instruction(0x5000);
        exception_thrown = false;
    } catch (std::exception& e) { }
    if(!exception_thrown || state.pc > 0xFFF){
        throw std::runtime_error("RAM overflow exception not thrown on SE");
    }
}


/*
6xkk - LD Vx, byte
The interpreter puts the value kk into register Vx.
*/
void test_instr_LD(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    prog.run_instruction(0x6abb);
    if(state.regs[0xa] != 0xbb){
        throw std::runtime_error("Register not updated on LD");
    }
}

/*
7xkk - ADD Vx, byte
Set Vx = Vx + kk.
Adds the value kk to the value of register Vx, then stores the result in Vx. 
*/
/// TODO: check if carry flag VF should be set on overflow.
void test_instr_ADD(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    prog.run_instruction(0x7abb);
    if(state.regs[0xa] != 0xbb){
        throw std::runtime_error("Register not updated on ADD");
    }

    // Register overflow
    state.regs[0xb] = 0x1;
    prog.run_instruction(0x7bff);
    if(state.regs[0xb] != 0x0){
        throw std::runtime_error("Register did not overflow on ADD");
    }
}

/*
8xy0 - LD Vx, Vy
Set Vx = Vy.
Stores the value of register Vy in register Vx.
*/
void test_instr_LD_regs(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xb] = 0xff;
    state.regs[0xa] = 0x00;
    prog.run_instruction(0x8ab0);

    if(state.regs[0xa] != 0xff){
        throw std::runtime_error("Register not updated on LD-regs");
    }
}

/*
8xy1 - OR Vx, Vy
Set Vx = Vx OR Vy.
Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx.
*/
void test_instr_OR(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab1);
    if(state.regs[0xa] != (0xaa | 0xbb)){
        throw std::runtime_error("Register not updated on OR");
    }
}

/*
8xy2 - AND Vx, Vy
Set Vx = Vx AND Vy.
Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx.
*/
void test_instr_AND(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab2);
    if(state.regs[0xa] != (0xaa & 0xbb)){
        throw std::runtime_error("Register not updated on AND");
    }
}

/*
8xy3 - XOR Vx, Vy
Set Vx = Vx XOR Vy.
Performs a bitwise exclusive OR on the values of Vx and Vy,
then stores the result in Vx.
*/
void test_instr_XOR(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab3);
    if(state.regs[0xa] != (0xaa ^ 0xbb)){
        throw std::runtime_error("Register not updated on XOR");
    }
}


/*
8xy4 - ADD Vx, Vy
Set Vx = Vx + Vy, set VF = carry.
The values of Vx and Vy are added together.
If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
Only the lowest 8 bits of the result are kept, and stored in Vx.
*/
void test_instr_ADD_regs(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0x22;
    state.regs[0xb] = 0x33;
    prog.run_instruction(0x8ab4);
    if(state.regs[0xa] != (0x22 + 0x33)){
        throw std::runtime_error("Register not updated on ADD-regs");
    }
    if(state.regs[0xF] != 0x0){
        throw std::runtime_error("Carry flag set when no overflow on ADD-regs");
    }

    // Register overflow
    state.regs[0xa] = 0x01;
    state.regs[0xb] = 0xff;
    prog.run_instruction(0x8ab4);
    if(state.regs[0xa] != 0x0){
        throw std::runtime_error("Register did not overflow on ADD-regs");
    }
    if(state.regs[0xF] != 0x1){
        throw std::runtime_error("Carry flag not set on overflow on ADD-regs");
    }
}

/*
8xy5 - SUB Vx, Vy
Set Vx = Vx - Vy, set VF = NOT borrow.
If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
*/
void test_instr_SUB(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0x22;
    prog.run_instruction(0x8ab5);
    if(state.regs[0xa] != (0xaa - 0x22)){
        throw std::runtime_error("Register not updated on SUB");
    }
    if(state.regs[0xF] != 0x1){
        throw std::runtime_error("No-borrow flag unset when no borrow on SUB");
    }

    // Register overflow
    state.regs[0xa] = 0x00;
    state.regs[0xb] = 0x01;
    prog.run_instruction(0x8ab5);
    if(state.regs[0xa] != 0xff){
        throw std::runtime_error("Register did not underflow on SUB");
    }
    if(state.regs[0xF] != 0x0){
        throw std::runtime_error("No-borrow flag set when borrow on SUB");
    }
}

/*
8xy6 - SHR Vx {, Vy}
Set Vx = Vx SHR 1.
If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0.
Then Vx is divided by 2 (right-shifted).
Originally, this right-shifted the value of Vy and stored it on Vx.
*/
void test_instr_SHR(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0x10;
    prog.run_instruction(0x8a06);
    if(state.regs[0xa] != (0x10 >> 1)){
        throw std::runtime_error("Register did not update on SHR");
    }
    if(state.regs[0xF] != 0x0){
        throw std::runtime_error("Carry flag VF set when no carry on SHR");
    }

    state.regs[0xa] = 0x11;
    prog.run_instruction(0x8a06);
    if(state.regs[0xa] != (0x11 >> 1)){
        throw std::runtime_error("Register did not update on SHR");
    }
    if(state.regs[0xF] != 0x1){
        throw std::runtime_error("Carry flag VF unset when carry on SHR");
    }
}


/*
8xy7 - SUBN Vx, Vy
Set Vx = Vy - Vx, set VF = NOT borrow.
If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
*/
void test_instr_SUBN(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0xaa;
    state.regs[0xb] = 0xbb;
    prog.run_instruction(0x8ab7);
    if(state.regs[0xa] != (0xbb - 0xaa)){
        throw std::runtime_error("Register not updated on SUBN");
    }
    if(state.regs[0xF] != 0x1){
        throw std::runtime_error("No-borrow flag unset when no borrow on SUBN");
    }

    // Register overflow
    state.regs[0xa] = 0x01;
    state.regs[0xb] = 0x00;
    prog.run_instruction(0x8ab7);
    if(state.regs[0xa] != 0xff){
        throw std::runtime_error("Register did not underflow on SUBN");
    }
    if(state.regs[0xF] != 0x0){
        throw std::runtime_error("No-borrow flag set when borrow on SUBN");
    }
}


/*
8xyE - SHL Vx {, Vy}
Set Vx = Vx SHL 1.
If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
Then Vx is multiplied by 2 (left-shifted).
Originally, this left-shifted the value of Vy and stored it on Vx.
*/
void test_instr_SHL(){
    auto prog = CHIP8::Interpreter();
    auto& state = prog.get_state();
    state.reset();

    state.regs[0xa] = 0x01;
    prog.run_instruction(0x8a0E);
    if(state.regs[0xa] != (0x01 << 1)){
        throw std::runtime_error("Register did not update on SHL");
    }
    if(state.regs[0xF] != 0x0){
        throw std::runtime_error("Carry flag VF set when no carry on SHL");
    }

    state.regs[0xa] = 0x80;
    prog.run_instruction(0x8a0E);
    if(state.regs[0xa] != CHIP8::byte_t(0x80 << 1)){
        throw std::runtime_error("Register did not update on carry on SHL");
    }
    if(state.regs[0xF] != 0x1){
        throw std::runtime_error("Carry flag VF unset when carry on SHL");
    }
}


/*
9xy0 - SNE Vx, Vy
Skip next instruction if Vx != Vy.
The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
*/


int main(int argc, const char* argv[]) {

    // test_state_reset();
    test_instr_RET();
    test_instr_JMP();
    test_instr_CALL();
    test_instr_SE();
    test_instr_SNE();
    test_instr_SE_regs();
    test_instr_LD();
    test_instr_LD_regs();
    test_instr_ADD();
    test_instr_OR();
    test_instr_AND();
    test_instr_XOR();
    test_instr_ADD_regs();
    test_instr_SUB();
    test_instr_SHR();
    test_instr_SUBN();
    test_instr_SHL();

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