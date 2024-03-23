
#include "chip8/chip8.h"

#include "chip8/renderer.h"

int main(int argc, const char* argv[]) {
    
    if(argc != 2){
        std::cout << 
        "Supply filename" << std::endl;
        return 1;
    }
    
    auto chip8 = CHIP8::Interpreter();
    chip8.load_file(argv[1]);
    chip8.run();

    /*
    auto renderer = CHIP8::Renderer();
    renderer.init();
    renderer.set_key_callback([&](int keycode){
        std::cout << keycode << std::endl;
    });
    while(renderer.is_running()){
        renderer.update();
    }
    */
}