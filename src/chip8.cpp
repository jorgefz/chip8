#include "chip8.h"

namespace CHIP8 {


    State* Interpreter::get_state(){
        return &this->m_state;
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
        
        // Set program counter to beginning of program
        m_state.pc = 0x200; // or 0x600 on ETI systems
    }

    void Interpreter::load_bytes(std::vector<byte_t> program){
        if(program.size() > RAM_SIZE - RAM_PROG_OFFSET){
            throw std::runtime_error("Program is too large");
        }
        std::copy_n(program.begin(), program.size(), m_state.ram.begin() + RAM_PROG_OFFSET);
        
        // Set program counter to beginning of program
        m_state.pc = 0x200; // or 0x600 on ETI systems
    }

    void Interpreter::run(){
        sf::VideoMode mode(SCREEN_WIDTH, SCREEN_HEIGHT);
        sf::RenderWindow window(mode, "CHIP8");

        m_canvas.create(NATIVE_WIDTH, NATIVE_HEIGHT, sf::Color::Black);
        m_texture.loadFromImage(m_canvas);
        m_sprite.setTexture(m_texture, true);
        m_sprite.setScale(SCREEN_SCALE, SCREEN_SCALE);

        draw_byte(32, 32, 0b10101010);

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            window.clear();
            window.draw(m_sprite);
            window.display();
        }
    }

    void Interpreter::draw_byte(byte_t x, byte_t y, byte_t byte){
        // draws 8 pixels from X=x to X=x+8, at constant Y=y.
        for(byte_t i = 0; i != 8; ++i){
            draw_pixel(x + i, y, (byte >> i) & 1);
        }
    }

    void Interpreter::draw_pixel(uint8_t x, uint8_t y, bool black){
        // Calculate color
        bool pixel = m_canvas.getPixel(x, y).r > 0;
        pixel ^= black;
        auto color = pixel ? sf::Color::Black : sf::Color::White;
        // Calculate position
        x %= NATIVE_WIDTH;
        y %= NATIVE_HEIGHT;
        // Draw pixel
        m_canvas.setPixel(x, y, color);
        m_texture.update(m_canvas);
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
                    // Clear display
                } else if (code == 0x00EE){
                    // Return from subroutine
                    if(m_state.sp == 0){
                        throw std::runtime_error(
                            "Cannot return from outside a subroutine"
                        );
                    }
                    m_state.pc = m_state.stack[m_state.sp];
                    m_state.sp--;
                }
                break;
            // JMP
            case 0x1:
                m_state.pc = addr;
                break;
            // CALL
            case 0x2:
                if(m_state.sp + 1 == STACK_SIZE){
                    throw std::runtime_error(
                        "Stack overflow. Too many subroutine calls!"
                    );
                }
                m_state.sp++;
                m_state.stack[m_state.sp] = m_state.pc;
                m_state.pc = addr;
                break;
            // Skip if
            case 0x3:
                if(m_state.regs[nib3] == low_byte){
                    m_state.pc += 2;
                }
                break;
            // Skip if not
            case 0x4:
                if(m_state.regs[nib3] != low_byte){
                    m_state.pc += 2;
                }
                break;
            // Skip if registers are equal
            case 0x5:
                if(m_state.regs[nib2] == m_state.regs[nib3]){
                    m_state.pc += 2;
                }
                break;
            // Set register to byte
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
                        m_state.regs[0xF] = m_state.regs[nib3] + m_state.regs[nib2] > 0xFF;
                        m_state.regs[nib3] += m_state.regs[nib2];
                        break;
                    case 0x5: // SUB
                        // Note: flag is set if no underflow
                        m_state.regs[0xF] = !(
                            m_state.regs[nib3] - m_state.regs[nib2] > m_state.regs[nib3]
                        );
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
                        m_state.regs[0xF] = !!(m_state.regs[nib3] & (0x2000));
                        m_state.regs[nib3] <<= 1;  // *= 2;
                        break;
                    default:
                        break;
                }
                break;
            // Skip if registers not equal
            case 0x9:
                if(m_state.regs[nib2] != m_state.regs[nib3]){
                    m_state.pc += 2;
                }
                break;
            // Set address pointer
            case 0xA:
                m_state.Ireg = addr;
                break;
            // JMP w/ offset
            case 0xB:
                m_state.pc = addr + m_state.regs[0x0];
                break;
            // RAND & reg
            case 0xC:
                m_state.regs[nib3] = static_cast<byte_t>(std::rand() % 0xFF) & low_byte;
                break;
            // Display sprite
            case 0xD: {
                // TO-DO
                /*
                The interpreter reads n bytes from memory,
                starting at the address stored in I.
                These bytes are then displayed as sprites on screen
                at coordinates (Vx, Vy).
                Sprites are XORed onto the existing screen.
                If this causes any pixels to be erased, VF is set to 1,
                otherwise it is set to 0.
                If the sprite is positioned so part of it is outside
                the coordinates of the display,
                it wraps around to the opposite side of the screen.
                */
                byte_t x = m_state.regs[nib3];
                byte_t y = m_state.regs[nib2];
                for(byte_t i = 0; i != nib1; ++i){
                    byte_t *p = m_state.ram.data() + m_state.Ireg + i;
                }
                break;
            }
            // Skip if key
            case 0xE:
                // TO-DO
                break;
            // Misc
            case 0xF:
                switch(low_byte){
                    case 0x07:
                        m_state.regs[nib3] = m_state.DTreg;
                        break;
                    case 0x0A:
                        // Halt execution until key press
                        // TODO
                        break;
                    case 0x15:
                        m_state.DTreg = m_state.regs[nib3];
                        break;
                    case 0x18:
                        m_state.STreg = m_state.regs[nib3];
                        break;
                    case 0x1E:
                        m_state.Ireg += m_state.regs[nib3];
                        break;
                    case 0x29:
                        // Get location of sprite
                        // TO-DO
                        break;
                    case 0x33:
                        m_state.ram[m_state.Ireg]   = (m_state.regs[nib3]/100) % 10;
                        m_state.ram[m_state.Ireg+1] = (m_state.regs[nib3]/10)  % 10;
                        m_state.ram[m_state.Ireg+2] =  m_state.regs[nib3]      % 10;
                        break;
                    case 0x55:
                        for(uint8_t i = 0x0; i != 0xF+1; ++i){
                            m_state.ram[m_state.Ireg + i] = m_state.regs[i];
                        }
                        break;
                    case 0x65:
                        for(uint8_t i = 0x0; i != 0xF+1; ++i){
                            m_state.regs[i] = m_state.ram[m_state.Ireg + i];
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}