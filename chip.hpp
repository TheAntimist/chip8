#include <SDL2/SDL.h>
#include <SDL2/SDL_endian.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <random>
#include <unordered_map>

// Global Constants
const unsigned int FONTSET_SIZE = 80, FONTSET_START_ADDRESS = 0x50,
                   VIDEO_WIDTH = 64, VIDEO_HEIGHT = 32;

unsigned int COLOR_DISABLED = 0xff2d9bc8, COLOR_ENABLED = 0xff14dce6;

std::uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/**
 * Main Class for Chip-8 Emulation
 */
class chip8 {
private:
  void opcode_0x8xyn(std::uint16_t);

  std::uint8_t registers[16]{};
  std::uint8_t memory[4096]{};
  std::uint16_t index{};
  std::uint16_t pc{};
  std::uint16_t stack[16]{};
  std::uint8_t sp{};
  std::uint8_t delayTimer{};
  std::uint8_t soundTimer{};
  std::uint16_t opcode;

  /**
   * Initializes all variables and loads the given file name contents into
   * memory for execution.
   */
  void init(std::string filename, bool q);

public:
  bool keypad[16]{};
  std::unordered_map<SDL_Keycode, int> keymap{};
  std::uint32_t display[VIDEO_WIDTH * VIDEO_HEIGHT]{};

  bool draw = false, quirky = false;

  /**
   * Loads the given file name contents into memory for execution
   */
  void loadFile(std::string);
  /**
   * Emulates a single CPU cycle based on the inputs
   */
  void emulateCycle();

  // Randomization routines
  std::random_device rd;
  std::mt19937 mt = std::mt19937(rd());
  std::uniform_int_distribution<std::uint8_t> dist =
      std::uniform_int_distribution<std::uint8_t>(0, 0xffu);

  chip8();
  chip8(std::string filename, bool quirky);
  void setQuirkyMode(bool);
};

void chip8::setQuirkyMode(bool q) { quirky = q; }

/**
 * Class for SDL Video manipulation
 */
class sdlvideo {
private:
  SDL_Window *win;
  SDL_Renderer *ren;
  SDL_Texture *texture;

public:
  sdlvideo(const int scale);
  ~sdlvideo();

  /**
   * Updates the texture object with the new pixel values
   */
  void updateTexture(std::uint32_t *pixels);
  /**
   * \brief Sets the key state based on the keys pressed. Also resets it before
   * setting. Note: Returns true if we have to exit.
   */
  bool setKeyStates(std::unordered_map<SDL_Keycode, int> keymap, bool keys[16]);
};
