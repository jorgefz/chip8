# chip8
This is a CHIP8 interpreter written in C++. It runs games that use the CHIP8 instruction set.

CHIP8 is an assembly-like programming language that typically runs on a virtual machine.
It was created in the 70s and became popular for running small retro games.
See https://en.wikipedia.org/wiki/CHIP-8 for more information.

## Usage
```
$ chip8 my_game.ch8
```

## Dependencies
* CMake: build system. See https://cmake.org/
* SFML: graphics library. See https://www.sfml-dev.org/
* Catch2: C++ test framework. See https://github.com/catchorg/Catch2

## Build instructions
```
git clone https://github.com/jorgefz/chip8
cd chip8
cmake -S . -B build
cd build
make
```