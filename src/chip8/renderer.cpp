#include "renderer.h"
#include <cstdint>

namespace CHIP8 {
    
    /* Creates a window */
    void Renderer::init(){
        if(m_running){
            throw std::runtime_error("Window already open");
        }

        // Initialise window
        sf::VideoMode mode(SCREEN_WIDTH, SCREEN_HEIGHT);
        m_window = std::make_unique<sf::RenderWindow>(mode, "CHIP8");

        // Setup program display/canvas
        m_canvas.create(NATIVE_WIDTH, NATIVE_HEIGHT, m_theme.first);
        m_texture.loadFromImage(m_canvas);
        m_sprite.setTexture(m_texture, true);
        m_sprite.setScale(SCREEN_SCALE, SCREEN_SCALE);
        clear_canvas();
        m_running = true;
        m_clock.restart();
    }

    /* True if the window is open */
    bool Renderer::is_running(){
        return m_running;
    }

    /* Polls events, updates canvas, and returns
    frame time in milliseconds */
    double Renderer::update(){
        if(!m_running){
            throw std::runtime_error("Window has not been initialised");
        }

        sf::Event event;
        while (m_window->pollEvent(event)) {
            if(event.type == sf::Event::Closed){
                m_running = false;
                m_window->close();
            }
        }
        
        m_window->clear();
        m_texture.update(m_canvas);
        m_window->draw(m_sprite);
        m_window->display();
        process_input();

        return m_clock.restart().asMilliseconds();
    }

    /* Draws a byte onto the canvas using the bits as pixels.
    Returns True if a pixel was overwritten. */
    bool Renderer::draw_byte(byte_t x, byte_t y, byte_t byte){
        // draws 8 pixels from X=x to X=x+8, at constant Y=y.
        bool collision = false;
        for(byte_t i = 0; i != 8; ++i){
            collision |= draw_pixel(x + 7 - i, y, byte & (1 << i));
        }
        return collision;
    }
    
    /* Draws (XORs) a single pixel onto the canvas at position (x,y).
    Returns True if a pixel was overwritten. */
    bool Renderer::draw_pixel(uint8_t x, uint8_t y, bool pixel){
        sf::Color color;
        bool collision = false;
        
        // Wrap around screen
        x %= NATIVE_WIDTH;
        y %= NATIVE_HEIGHT;
        
        // Calculate color
        pixel ^= (m_canvas.getPixel(x, y).r > 0);
        color = pixel ? m_theme.second : m_theme.first;
        
        if( (m_canvas.getPixel(x, y).r > 0) && pixel){
            collision = true;
        }

        // Draw pixel
        m_canvas.setPixel(x, y, color);
        return collision;
    }

    void Renderer::clear_canvas(){
        for(int i = 0; i != NATIVE_WIDTH; ++i){
            for(int j = 0; j != NATIVE_HEIGHT; ++j){
                m_canvas.setPixel(i, j, m_theme.first);
            }
        }
    }

    /* Defines the two colors used on the canvas */
    void Renderer::set_theme(sf::Color primary, sf::Color secondary){
        m_theme.first  = primary;
        m_theme.second = secondary;
    }

    /* Query keypad for keys */
    void Renderer::process_input(){
        for(uint16_t key = 0x0; key != 0x10; ++key){
            m_keypad[key] = sf::Keyboard::isKeyPressed(m_key_bindings[key]);
        }
    }

    /* Returns true if a keypad key is being pressed */
    bool Renderer::is_key_pressed(byte_t key){
        return m_keypad[key];
    }
}