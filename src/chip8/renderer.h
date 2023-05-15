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
        std::function<void(int)> m_key_callback;

    public:
        Renderer()
            : m_running(false),
              m_theme(sf::Color::Black, sf::Color::White) { }
        
        ~Renderer() { }

        /* Creates a window */
        void init();

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

        /* Fills out the canvas with ... color */
        void clear_canvas();

        /* Defines the two colors used on the canvas */
        void set_theme(sf::Color bright, sf::Color dark);

        /* Function called when key event is propagated */
        void set_key_callback(std::function<void(int)> callback){
            m_key_callback = callback;
        }
    };
}


#endif /* CHIP8_RENDERER_H */