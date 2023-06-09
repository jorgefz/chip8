#include "chip8.h"
#include <cassert>
#include <sstream>

static void debug_error(uint16_t code, std::string msg){
    std::stringstream error_msg;
    error_msg << std::hex << "[0x"<<code<<"]" << msg << std::endl;
    throw std::runtime_error(error_msg.str().c_str());
}

namespace CHIP8 {

    Interpreter::Interpreter(){
        m_state.reset();
        // Set program counter to beginning of program
        m_state.pc = 0x200; // or 0x600 on ETI systems
        m_rng.seed(std::time(nullptr));
        m_timer = 0.0;
        m_timer_freq = 60.0; // Hz
        m_halt_until_key = false;
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

        m_renderer.set_key_callback([&](int keycode){
            if(m_halt_until_key){
                check_halt_key(static_cast<sf::Keyboard::Key>(keycode));
            }
        });

        m_renderer.init();

        double dt;
        while (m_renderer.is_running()) {
            dt = m_renderer.update();
            update_timers(dt);

            if(m_halt_until_key) continue;
            uint16_t code = (m_state.ram[m_state.pc] << 8) | m_state.ram[m_state.pc+1];
            m_state.pc += 2;
            run_instruction(code);

            if(m_state.pc >= CHIP8::RAM_SIZE){
                debug_error(code, "RAM overflow");
            }
        }
    }

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

    void Interpreter::check_halt_key(sf::Keyboard::Key keycode){
        auto found = std::find_if(
            key_bindings.begin(),
            key_bindings.end(),
            [&](const auto& pair){return pair.second == keycode;}
        );
        if(found == key_bindings.end()) return;
        m_state.regs[m_reg_store_key] = found->first;
        m_halt_until_key = false;
        m_state.pc += 2;
    }

    void Interpreter::run_instruction(uint16_t code){
        
        // Nibbles
        byte_t nib1 = (code & 0x000F);
        byte_t nib2 = (code & 0x00F0) >> 4;
        byte_t nib3 = (code & 0x0F00) >> 8;
        byte_t high_nibble = (code & 0xF000) >> 12;
        byte_t low_byte = (code & 0x00FF);
        uint16_t addr = (code & 0x0FFF);
        
        switch(high_nibble) {
            case 0x0:
                if(code == 0x00E0) m_renderer.clear_canvas(); // CLS
                else if (code == 0x00EE){ // RET
                    assert(m_state.sp > 0);
                    m_state.pc = m_state.stack[--m_state.sp];
                }
                break;
            case 0x1: m_state.pc = addr; break; // JMP
            case 0x2: // CALL
                assert(m_state.sp <= STACK_SIZE);
                m_state.stack[++m_state.sp-1] = m_state.pc + 2;
                m_state.pc = addr;
                break;
            case 0x3: // SE
                if(m_state.regs[nib3] == low_byte) m_state.pc += 2;
                break;
            case 0x4: // SNE
                if(m_state.regs[nib3] != low_byte) m_state.pc += 2;
                break;
            case 0x5: // SE
                if(m_state.regs[nib2] == m_state.regs[nib3]) m_state.pc += 2;
                break;
            case 0x6: m_state.regs[nib3]  = low_byte; break; // LD
            case 0x7: m_state.regs[nib3] += low_byte; break; // ADD
            case 0x8: // Bitwise/arithmetic operations
                switch(nib1){
                    case 0x0: m_state.regs[nib3]  = m_state.regs[nib2]; break; // LD
                    case 0x1: m_state.regs[nib3] |= m_state.regs[nib2]; break; // OR
                    case 0x2: m_state.regs[nib3] &= m_state.regs[nib2]; break; // AND
                    case 0x3: m_state.regs[nib3] ^= m_state.regs[nib2]; break; // XOR
                    case 0x4: // ADD
                        m_state.regs[0xF] = (m_state.regs[nib3] + m_state.regs[nib2]) > 0xFF;
                        m_state.regs[nib3] += m_state.regs[nib2];
                        break;
                    case 0x5: // SUB (VF = NO BORROW)
                        m_state.regs[0xF] = !(m_state.regs[nib2] > m_state.regs[nib3]);
                        m_state.regs[nib3] -= m_state.regs[nib2];
                        break;
                    case 0x6: // SHR
                        m_state.regs[0xF] = (m_state.regs[nib3] & 0x1);
                        m_state.regs[nib3] >>= 1;
                        break;
                    case 0x7: // SUBN
                        m_state.regs[0xF] = (m_state.regs[nib2] > m_state.regs[nib3]);
                        m_state.regs[nib3] = m_state.regs[nib2] - m_state.regs[nib3];
                        break;
                    case 0xE: // SHL
                        m_state.regs[0xF] = (m_state.regs[nib3] & (1 << 7)) != 0x0;
                        m_state.regs[nib3] <<= 1;
                        break;
                }
                break;
            case 0x9: // SNE
                if(m_state.regs[nib2] != m_state.regs[nib3]) m_state.pc += 2;
                break;
            case 0xA: m_state.Ireg = addr; break; // LD
            case 0xB: m_state.pc = addr + m_state.regs[0x0]; break; // JMP
            case 0xC: m_state.regs[nib3] = random_byte() & low_byte; break; // RND
            case 0xD: { // DRW
                byte_t x = m_state.regs[nib3];
                byte_t y = m_state.regs[nib2];
                m_state.regs[0xF] = 0;
                if(m_state.Ireg + nib1 > RAM_SIZE){
                    debug_error(code,
                        "RAM overflow. I register out of bounds when retrieving sprite");
                }
                for(byte_t i = 0; i != nib1; ++i){
                    byte_t sprite_line = m_state.ram[m_state.Ireg + i];
                    m_state.regs[0xF] = m_renderer.draw_byte(x, y + i, sprite_line);
                }
                break;
            }
            case 0xE: // Key input
                switch(low_byte){
                    case 0x9E: // Skip if key pressed
                        if(sf::Keyboard::isKeyPressed(key_bindings.at(nib3))) m_state.pc += 2;
                        break;
                    case 0xA1: // Skip if key not pressed
                        if(!sf::Keyboard::isKeyPressed(key_bindings.at(nib3))) m_state.pc += 2;
                        break;
                }
                break;
            case 0xF: // Misc
                switch(low_byte){
                    case 0x07:  m_state.regs[nib3] = m_state.DTreg; break; //LD
                    case 0x0A: // Halt execution until key press
                        m_halt_until_key = true;
                        m_reg_store_key = nib3;
                        break;
                    case 0x15: m_state.DTreg = m_state.regs[nib3]; break; // LD
                    case 0x18: m_state.STreg = m_state.regs[nib3]; break; // LD
                    case 0x1E: m_state.Ireg += m_state.regs[nib3]; break; // ADD
                    case 0x29: m_state.Ireg = m_state.regs[nib3] * 5; break; // get digit
                    case 0x33: // BCD
                        m_state.ram[m_state.Ireg]   = (m_state.regs[nib3]/100) % 10;
                        m_state.ram[m_state.Ireg+1] = (m_state.regs[nib3]/10)  % 10;
                        m_state.ram[m_state.Ireg+2] =  m_state.regs[nib3]      % 10;
                        break;
                    case 0x55: // LD
                        for(byte_t i = 0x0; i != nib3 + 1; ++i){
                            m_state.ram[m_state.Ireg + i] = m_state.regs[i];
                        }
                        break;
                    case 0x65: // LD
                        for(byte_t i = 0x0; i != 0xF+1; ++i){
                            m_state.regs[i] = m_state.ram[m_state.Ireg + i];
                        }
                        break;
                }
                break;
        }
    }
}