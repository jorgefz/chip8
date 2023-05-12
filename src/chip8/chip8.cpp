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
        m_increment_pc = true;
    }

    State& Interpreter::get_state(){
        return this->m_state;
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
        sf::VideoMode mode(SCREEN_WIDTH, SCREEN_HEIGHT);
        sf::RenderWindow window(mode, "CHIP8");

        // Setup program display/canvas
        m_canvas.create(NATIVE_WIDTH, NATIVE_HEIGHT, sf::Color::Black);
        m_texture.loadFromImage(m_canvas);
        m_sprite.setTexture(m_texture, true);
        m_sprite.setScale(SCREEN_SCALE, SCREEN_SCALE);
        clear_canvas();

        double dt;
        while (window.isOpen()) {
            dt = m_clock.restart().asMilliseconds();
            update_timers(dt);

            sf::Event event;
            while (window.pollEvent(event)) {
                switch(event.type){
                    case sf::Event::Closed:
                        window.close();
                        break;
                    case sf::Event::KeyPressed:
                        if(m_halt_until_key){
                            check_halt_key(event.key.code);
                        }
                        break;
                }
            }
            
            window.clear();
            m_texture.update(m_canvas);
            window.draw(m_sprite);
            window.display();

            if(m_state.pc == CHIP8::RAM_SIZE){
                continue;
            }

            if(m_halt_until_key){
                std::cout << "Halted " << std::endl;
                continue;
            }

            m_increment_pc = true;
            uint16_t code = (m_state.ram[m_state.pc] << 8) | m_state.ram[m_state.pc+1];
            run_instruction(code);
            if(m_increment_pc){
                m_state.pc += 2;
            }
        }
    }

    void Interpreter::draw_byte(byte_t x, byte_t y, byte_t byte){
        // draws 8 pixels from X=x to X=x+8, at constant Y=y.
        /// TODO: ensure draw order for x-coordinate is correct
        for(byte_t i = 0; i != 8; ++i){
            draw_pixel(x + 8-i, y, byte & (1 << i));
        }
    }

    void Interpreter::draw_pixel(uint8_t x, uint8_t y, bool pixel){
        // Calculate position
        x %= NATIVE_WIDTH;
        y %= NATIVE_HEIGHT;
        // Calculate color
        pixel ^= (m_canvas.getPixel(x, y).r > 0);
        auto color = pixel ? sf::Color::White : sf::Color::Black;
        // Set VF if pixel erased
        if( (m_canvas.getPixel(x, y).r > 0) && pixel){
            m_state.regs[0xF] = 1;
        }
        // Draw pixel
        m_canvas.setPixel(x, y, color);
    }

    void Interpreter::clear_canvas(){
        for(int i = 0; i != NATIVE_WIDTH; ++i){
            for(int j = 0; j != NATIVE_HEIGHT; ++j){
                m_canvas.setPixel(i, j, sf::Color::Black);
            }
        }
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

    void Interpreter::check_halt_key(sf::Keyboard::Key keycode){
        const sf::Keyboard::Key* key = std::find(
            key_bindings.begin(),
            key_bindings.end(),
            keycode
        );
        if (key == key_bindings.end()){
            return;
        }
        m_state.regs[m_reg_store_key] = static_cast<byte_t>(std::distance(key_bindings.begin(), key));
        m_halt_until_key = false;
    }

    void Interpreter::run_instruction(uint16_t code){
        
        // Nibbles
        byte_t nib1 = (code & 0x000F),
               nib2 = (code & 0x00F0) >> 4,
               nib3 = (code & 0x0F00) >> 8,
               nib4 = (code & 0xF000) >> 12;
        
        byte_t low_byte = (code & 0x00FF);
        
        // The least significant tribble (12bit) value
        // is typically used as a memory address
        uint16_t addr = (code & 0x0FFF);
        
        switch(nib4) {
            // Misc - general state
            case 0x0:
                if(code == 0x00E0){
                    // CLS - clear display
                    clear_canvas();
                } else if (code == 0x00EE){
                    // RET - return from subroutine
                    if(m_state.sp == 0){
                        throw std::runtime_error(
                            "Cannot return from outside a subroutine"
                        );
                    }
                    m_state.pc = m_state.stack[m_state.sp-1];
                    m_state.sp--;
                    m_increment_pc = false;
                }
                break;
            // JMP
            case 0x1:
                m_state.pc = addr;
                break;
            // CALL
            case 0x2:
                if(m_state.sp == STACK_SIZE){
                    throw std::runtime_error(
                        "Stack overflow. Too many subroutine calls!"
                    );
                }
                m_state.sp++;
                m_state.stack[m_state.sp-1] = m_state.pc + 2;
                m_state.pc = addr;
                m_increment_pc = false;
                break;
            // SE (skip if)
            case 0x3:
                if(m_state.regs[nib3] != low_byte){
                    break;
                }
                if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                    debug_error(code, "RAM overflow - jumped too far");
                }
                m_state.pc += 2;
                m_increment_pc = false;
                break;
            // SNE (skip if not)
            case 0x4:
                if(m_state.regs[nib3] == low_byte){
                    break;
                }
                if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                    debug_error(code, "RAM overflow - jumped too far");
                }
                m_state.pc += 2;
                m_increment_pc = false;
                break;
            // SE-regs (skip if registers are equal)
            case 0x5:
                if(m_state.regs[nib2] != m_state.regs[nib3]){
                    break;
                }
                if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                    debug_error(code, "RAM overflow - jumped too far");
                }
                m_state.pc += 2;
                m_increment_pc = false;
                break;
            // LD - Set register to byte
            case 0x6:
                m_state.regs[nib3] = low_byte;
                break;
            // ADD
            case 0x7:
                m_state.regs[nib3] += low_byte;
                break;
            // Registry operations
            case 0x8:
                switch(nib1){
                    case 0x0: // SET
                        m_state.regs[nib3] = m_state.regs[nib2];
                        break;
                    case 0x1: // OR
                        m_state.regs[nib3] |= m_state.regs[nib2];
                        break;
                    case 0x2: // AND
                        m_state.regs[nib3] &= m_state.regs[nib2];
                        break;
                    case 0x3: // XOR
                        m_state.regs[nib3] ^= m_state.regs[nib2];
                        break;
                    case 0x4: // ADD 
                        m_state.regs[0xF] = (m_state.regs[nib3] + m_state.regs[nib2]) > 0xFF;
                        m_state.regs[nib3] += m_state.regs[nib2];
                        break;
                    case 0x5: // SUB
                        // Note: VF flag is set if no underflow
                        m_state.regs[0xF] = !(m_state.regs[nib2] > m_state.regs[nib3]);
                        m_state.regs[nib3] -= m_state.regs[nib2];
                        break;
                    case 0x6: // SHR
                        m_state.regs[0xF] = (m_state.regs[nib3] & 0x1);
                        m_state.regs[nib3] >>= 1;  // /= 2;
                        break;
                    case 0x7: // SUBN
                        m_state.regs[0xF] = (m_state.regs[nib2] > m_state.regs[nib3]);
                        m_state.regs[nib3] = m_state.regs[nib2] - m_state.regs[nib3];
                        break;
                    case 0xE: // SHL
                        m_state.regs[0xF] = (m_state.regs[nib3] & (1 << 7)) != 0x0;
                        m_state.regs[nib3] <<= 1;  // *= 2;
                        break;
                    default:
                        break;
                }
                break;
            // Skip if registers not equal
            case 0x9:
                if(m_state.regs[nib2] == m_state.regs[nib3]){
                    break;
                }
                if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                    debug_error(code, "RAM overflow - jumped too far");
                }
                m_state.pc += 2;
                m_increment_pc = false;
                break;
            // Set address pointer
            case 0xA:
                m_state.Ireg = addr;
                break;
            // JMP w/ offset
            case 0xB:
                if(addr + m_state.regs[0x0] > 0xFFF){
                    debug_error(code, "RAM overflow - jumped too far");
                }
                m_state.pc = addr + m_state.regs[0x0];
                break;
            // RAND & reg
            case 0xC:
                m_state.regs[nib3] = random_byte() & low_byte;
                break;
            // Display sprite
            case 0xD: {
                byte_t x = m_state.regs[nib3];
                byte_t y = m_state.regs[nib2];
                m_state.regs[0xF] = 0;
                if(m_state.Ireg+nib1 > RAM_SIZE){
                    debug_error(code, "RAM overflow -  'I' register out of bounds when retrieving sprite");
                }
                for(byte_t i = 0; i != nib1; ++i){
                    byte_t sprite_line = m_state.ram[m_state.Ireg + i];
                    draw_byte(x, y + i, sprite_line);
                    /// TODO: ensure y-coordinate draw direction is correct
                }
                break;
            }
            // Skip if key
            case 0xE:
                switch(low_byte){
                    case 0x9E: // Skip if key pressed
                        if(!sf::Keyboard::isKeyPressed(key_bindings[nib3])){
                            break;
                        }
                        if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                            debug_error(code, "RAM overflow - jumped too far");
                        }
                        m_state.pc += 2;
                        m_increment_pc = false;
                        break;
                    case 0xA1: // Skip if key not pressed
                        if(sf::Keyboard::isKeyPressed(key_bindings[nib3])){
                            break;
                        }
                        if(m_state.pc + 2 >= CHIP8::RAM_SIZE){
                            debug_error(code, "RAM overflow - jumped too far");
                        }
                        m_state.pc += 2;
                        m_increment_pc = false;
                        break;
                    default:
                        break;
                }
                break;
            // Misc
            case 0xF:
                switch(low_byte){
                    case 0x07: // Get DT
                        m_state.regs[nib3] = m_state.DTreg;
                        break;
                    case 0x0A: // Halt execution until key press
                        m_halt_until_key = true;
                        m_reg_store_key = nib3;
                        break;
                    case 0x15: // Set DT
                        m_state.DTreg = m_state.regs[nib3];
                        break;
                    case 0x18: // Set ST
                        m_state.STreg = m_state.regs[nib3];
                        break;
                    case 0x1E: // Increase I
                        m_state.Ireg += m_state.regs[nib3];
                        break;
                    case 0x29:
                        // Get location of font sprite
                        // representing value at nib3
                        m_state.Ireg = m_state.regs[nib3] * 5; // Each hex sprite is 5 bytes long
                        break;
                    case 0x33: // Store BCD representation
                        m_state.ram[m_state.Ireg]   = (m_state.regs[nib3]/100) % 10;
                        m_state.ram[m_state.Ireg+1] = (m_state.regs[nib3]/10)  % 10;
                        m_state.ram[m_state.Ireg+2] =  m_state.regs[nib3]      % 10;
                        break;
                    case 0x55: // Save registers to RAM
                        for(byte_t i = 0x0; i != nib3 + 1; ++i){
                            m_state.ram[m_state.Ireg + i] = m_state.regs[i];
                        }
                        break;
                    case 0x65: // Load registers from RAM
                        for(byte_t i = 0x0; i != 0xF+1; ++i){
                            m_state.regs[i] = m_state.ram[m_state.Ireg + i];
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                std::cout << "Invalid instruction 0x" << std::hex << code << std::endl;
                std::runtime_error(" ");
                break;
        }
    }
}