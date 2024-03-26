#include "chip8.h"
#include <sstream>

namespace CHIP8 {

    Interpreter::Interpreter(){
        m_state.reset();
        // Set program counter to beginning of program
        m_state.pc = 0x200; // or 0x600 on ETI systems
        m_rng.seed(std::time(nullptr));
        m_timer = 0.0;
        m_timer_freq = 60.0; // Hz
    }

    void Interpreter::load_file(std::string filename){
        std::ifstream input(filename, std::ios::binary);
        if(!input){
            throw std::runtime_error("Input file not found");
        }
        
        // The contents are copied to the program region of the RAM
        input.read(
            reinterpret_cast<char*>(m_state.ram.data()) + RAM_PROG_OFFSET,
            RAM_SIZE - RAM_PROG_OFFSET
        );

        input.close();
    }

    void Interpreter::load_bytes(std::vector<byte_t> program){
        if(program.size() > RAM_SIZE - RAM_PROG_OFFSET){
            throw std::runtime_error("Program is too large");
        }
        std::copy_n(program.begin(), program.size(), m_state.ram.begin() + RAM_PROG_OFFSET);
    }

    void Interpreter::run(){
        // Initialise window
        double dt;
        m_renderer.init();

        while(m_renderer.is_running()){
            dt = m_renderer.update();
            update_timers(dt);

            uint16_t code = m_state.advance();
            run_instruction(code);
        }
    }

    void Interpreter::draw_byte(byte_t x, byte_t y, byte_t byte){
        bool drawn = m_renderer.draw_byte(x, y, byte);
        if(drawn) m_state.regs[0xF] = 1;
    }

    void Interpreter::draw_pixel(uint8_t x, uint8_t y, bool pixel){
       bool drawn = m_renderer.draw_pixel(x, y, pixel);
       if(drawn) m_state.regs[0xF] = 1;

    }

    /// TODO: set up random number generator
    byte_t Interpreter::random_byte(){
        std::uniform_int_distribution<byte_t> distribution(0x0, 0xFF);
        return distribution(m_rng);
    }

    void Interpreter::update_timers(double dt){
        m_timer += dt;
        if(m_timer < 1000.0/m_timer_freq){
            return;
        }
        m_timer = 0.0;
        if(m_state.DTreg != 0x0){
            m_state.DTreg -= 1;
        }

        if(m_state.STreg != 0x0){
            m_state.STreg -= 1;
        }
    }

    void Interpreter::run_instruction(uint16_t code){
        
        // Nibbles
        byte_t low_nib  = (code & 0x000F);
        byte_t vy       = (code & 0x00F0) >> 4;
        byte_t vx       = (code & 0x0F00) >> 8;
        byte_t high_nib = (code & 0xF000) >> 12;
        byte_t low_byte = (code & 0x00FF);
        uint16_t addr   = (code & 0x0FFF);
        
        switch(high_nib) {
            case 0x0:
                if(code == 0x00E0){ // CLS
                    m_renderer.clear_canvas();
                } else if (code == 0x00EE){ // RET
                    if(m_state.sp == 0){
                        throw std::runtime_error("No subroutine to return from");
                    }
                    m_state.sp--;
                    m_state.pc = m_state.stack[m_state.sp];
                }
                break;
            case 0x1: // JMP
                m_state.jump(addr);
                break;
            case 0x2: // CALL
                if(m_state.sp + 1 == STACK_SIZE){
                    throw std::runtime_error("Stack overflow: subroutine call limit reached");
                }
                m_state.stack[m_state.sp] = m_state.pc;
                m_state.sp++;
                m_state.pc = addr;
                break;
            case 0x3: // SE
                if(m_state.regs[vx] == low_byte){
                    m_state.advance();
                }
                break;
            case 0x4: // SNE
                if(m_state.regs[vx] != low_byte){
                    m_state.advance();
                }
                break;
            case 0x5: // SE
                if(m_state.regs[vy] == m_state.regs[vx]){
                    m_state.advance();
                }
                break;
            case 0x6: // LD
                m_state.regs[vx]  = low_byte;
                break;
            case 0x7: // ADD
                m_state.regs[vx] += low_byte;
                break;
            case 0x8: // Bitwise/arithmetic operations
                switch(low_nib){
                    case 0x0: m_state.regs[vx]  = m_state.regs[vy]; break; // LD
                    case 0x1: m_state.regs[vx] |= m_state.regs[vy]; break; // OR
                    case 0x2: m_state.regs[vx] &= m_state.regs[vy]; break; // AND
                    case 0x3: m_state.regs[vx] ^= m_state.regs[vy]; break; // XOR
                    case 0x4: // ADD
                        m_state.regs[0xF] = ((m_state.regs[vx] + m_state.regs[vy]) > 0xFF);
                        m_state.regs[vx] += m_state.regs[vy];
                        break;
                    case 0x5: // SUB (VF = NO BORROW)
                        m_state.regs[0xF] = (m_state.regs[vx] > m_state.regs[vy]);
                        m_state.regs[vx] -= m_state.regs[vy];
                        break;
                    case 0x6: // SHR
                        m_state.regs[0xF] = (m_state.regs[vx] & 0x1);
                        m_state.regs[vx] >>= 1;
                        // m_state.regs[vy] = m_state.regs[vx];
                        break;
                    case 0x7: // SUBN
                        m_state.regs[0xF] = (m_state.regs[vy] > m_state.regs[vx]);
                        m_state.regs[vx] = m_state.regs[vy] - m_state.regs[vx];
                        break;
                    case 0xE: // SHL
                        m_state.regs[0xF] = (m_state.regs[vx] & 0x80) >> 7;
                        m_state.regs[vx] <<= 1;
                        // m_state.regs[vy] = m_state.regs[vx];
                        break;
                }
                break;
            case 0x9: // SNE
                if(m_state.regs[vy] != m_state.regs[vx]){
                    m_state.advance();
                }
                break;
            case 0xA: // LD
                m_state.Ireg = addr;
                break;
            case 0xB: // JMP
                m_state.jump(addr + m_state.regs[0]);
                break;
            case 0xC: // RND
                m_state.regs[vx] = random_byte() & low_byte;
                break;
            case 0xD: { // DRW
                byte_t x = m_state.regs[vx];
                byte_t y = m_state.regs[vy];
                m_state.regs[0xF] = 0;
                if(m_state.Ireg + low_nib > RAM_SIZE){
                    throw std::runtime_error("RAM overflow when retrieving font sprite");
                }
                for(byte_t i = 0; i != low_nib; ++i){
                    byte_t sprite_line = m_state.ram[m_state.Ireg + i];
                    draw_byte(x, y + i, sprite_line);
                }
                break;
            }
            case 0xE: // Key input
                switch(low_byte){
                    case 0x9E: // Skip if key pressed
                        if(m_renderer.is_key_pressed(m_state.regs[vx])){
                            m_state.advance();
                        }
                        break;
                    case 0xA1: // Skip if key not pressed
                        if(!m_renderer.is_key_pressed(m_state.regs[vx])){
                            m_state.advance();
                        }
                        break;
                }
                break;
            case 0xF: // Misc
                switch(low_byte){
                    case 0x07:  m_state.regs[vx] = m_state.DTreg; break; //LD
                    case 0x0A: { // Halt execution until key press
                        bool key_pressed = false;
                        for(uint16_t key = 0x0; key != 0x10; ++key){
                            if(m_renderer.is_key_pressed(key)){
                                m_state.regs[vx] = byte_t(key);
                                key_pressed = true;
                                break;
                            }
                        }
                        if(!key_pressed){
                            m_state.pc -= 2; // Prevents program counter from advancing
                        }
                        break;
                    }
                    case 0x15: m_state.DTreg = m_state.regs[vx]; break; // LD
                    case 0x18: m_state.STreg = m_state.regs[vx]; break; // LD
                    case 0x1E: m_state.Ireg += m_state.regs[vx]; break; // ADD
                    case 0x29: m_state.Ireg = m_state.regs[vx] * 5; break; // get digit
                    case 0x33: // BCD
                        m_state.ram[m_state.Ireg+2] =  m_state.regs[vx]      % 10;;
                        m_state.ram[m_state.Ireg+1] = (m_state.regs[vx]/10)  % 10;
                        m_state.ram[m_state.Ireg]   = (m_state.regs[vx]/100) % 10;
                        break;
                    case 0x55: // LD
                        for(uint16_t i = 0x0; i <= vx; ++i){
                            m_state.ram[m_state.Ireg + i] = m_state.regs[i];
                        }
                        break;
                    case 0x65: // LD
                        for(uint16_t i = 0x0; i <= vx; ++i){
                            m_state.regs[i] = m_state.ram[m_state.Ireg + i];
                        }
                        break;
                }
                break;
            default:
                std::cout << "Invalid opcode " << std::hex << high_nib << std::endl;
                exit(0);
        }
    }
}