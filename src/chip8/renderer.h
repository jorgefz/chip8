#ifndef CHIP8_RENDERER_H
#define CHIP8_RENDERER_H

#include <SFML/Graphics.hpp>
#include "state.h"

namespace CHIP8 {

    class Renderer {
    
    public:
        static constexpr int NATIVE_WIDTH  = 64;
        static constexpr int NATIVE_HEIGHT = 32;
        static constexpr int SCREEN_SCALE  = 16;
        static constexpr int SCREEN_WIDTH  = NATIVE_WIDTH  * SCREEN_SCALE;
        static constexpr int SCREEN_HEIGHT = NATIVE_HEIGHT * SCREEN_SCALE;

    private:
        sf::Image   m_canvas;
        sf::Texture m_texture;
        sf::Sprite  m_sprite;
        sf::Clock   m_clock;
        std::unique_ptr<sf::RenderWindow> m_window;
        std::pair<sf::Color, sf::Color>   m_theme;
        bool m_running;
        std::array<bool, 0x10> m_keypad;
        const std::array<sf::Keyboard::Key, 0x10> m_key_bindings = {
            sf::Keyboard::Key::Num0,
            sf::Keyboard::Key::Num1,
            sf::Keyboard::Key::Num2,
            sf::Keyboard::Key::Num3,
            sf::Keyboard::Key::Num4,
            sf::Keyboard::Key::Num5,
            sf::Keyboard::Key::Num6,
            sf::Keyboard::Key::Num7,
            sf::Keyboard::Key::Num8,
            sf::Keyboard::Key::Num9,
            sf::Keyboard::Key::A   ,
            sf::Keyboard::Key::B   ,
            sf::Keyboard::Key::C   ,
            sf::Keyboard::Key::D   ,
            sf::Keyboard::Key::E   ,
            sf::Keyboard::Key::F   ,
        };

    public:
        Renderer()
            : m_running(false),
              m_theme(sf::Color::Black, sf::Color::White){
            std::fill(m_keypad.begin(), m_keypad.end(), false);
        }
        
        ~Renderer() { }

        /* Creates a window */
        void init();

        // Only for testing
        sf::Image& get_canvas() { return m_canvas; }

        /* True if the window is open */
        bool is_running();

        /* Polls events, updates canvas, and returns
        frame time in milliseconds */
        double update();

        /* Draws a byte onto the canvas using the bits as pixels.
        Returns True if a pixel was overwritten. */
        bool draw_byte(byte_t x, byte_t y, byte_t byte);

        /* Draws (XORs) a single pixel onto the canvas at position (x,y).
        Returns True if a pixel was overwritten. */
        bool draw_pixel(uint8_t x, uint8_t y, bool pixel);

        /* Fills out the canvas with black color */
        void clear_canvas();

        /* Defines the two colors used on the canvas */
        void set_theme(sf::Color bright, sf::Color dark);

        /* Query keypad for keys */
        void process_input();

        /* Returns true if a keypad key is being pressed */
        bool is_key_pressed(byte_t key);

    };
}


#endif /* CHIP8_RENDERER_H */