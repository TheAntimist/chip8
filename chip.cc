/**

 */

#include "chip.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <cstdint>

chip8::chip8() { init("", false); }

chip8::chip8(std::string filename, bool quirky) { init(filename, quirky); }

void chip8::init(std::string filename, bool q) {
  pc = 0x200;
  opcode = 0; // Reset current opcode
  index = 0;  // Reset index register
  sp = 0;     // Reset stack pointer

  // Load fonts into memory
  for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
    memory[FONTSET_START_ADDRESS + i] = fontset[i];
  }

  setQuirkyMode(q);

  // Set the keymap
  keymap.insert({SDL_GetKeyFromName("1"), 0});
  keymap.insert({SDL_GetKeyFromName("2"), 1});
  keymap.insert({SDL_GetKeyFromName("3"), 2});
  keymap.insert({SDL_GetKeyFromName("4"), 3});
  keymap.insert({SDL_GetKeyFromName("Q"), 4});
  keymap.insert({SDL_GetKeyFromName("W"), 5});
  keymap.insert({SDL_GetKeyFromName("E"), 6});
  keymap.insert({SDL_GetKeyFromName("R"), 7});
  keymap.insert({SDL_GetKeyFromName("A"), 8});
  keymap.insert({SDL_GetKeyFromName("S"), 9});
  keymap.insert({SDL_GetKeyFromName("D"), 10});
  keymap.insert({SDL_GetKeyFromName("F"), 11});
  keymap.insert({SDL_GetKeyFromName("Z"), 12});
  keymap.insert({SDL_GetKeyFromName("X"), 13});
  keymap.insert({SDL_GetKeyFromName("C"), 14});
  keymap.insert({SDL_GetKeyFromName("V"), 15});

  std::fill(std::begin(display),
            std::begin(display) + VIDEO_WIDTH * VIDEO_HEIGHT, COLOR_DISABLED);
  if (!filename.empty()) {
    loadFile(filename);
  }
}

void chip8::loadFile(std::string filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (file.is_open()) {
    std::streampos size = file.tellg();
    char *buffer = new char[size];

    file.seekg(0, std::ios::beg);
    file.read(buffer, size);

    for (long i = 0; i < size; i++) {
      // 0x200 = 512
      memory[512 + i] = buffer[i];
    }
    delete[] buffer;
  }
}

void chip8::emulateCycle() {
  const std::uint16_t opcode = (memory[pc] << 8u) | memory[pc + 1];

  // Next instruction
  pc += 2;
  draw = false;

  switch (opcode & 0xF000u) {
  case 0x00:
    if (opcode == 0x00E0u) {
      // CLS
      std::fill(std::begin(display),
                std::begin(display) + VIDEO_WIDTH * VIDEO_HEIGHT,
                COLOR_DISABLED);
      draw = true;
    } else if (opcode == 0x00EEu) {
      // RET
      pc = stack[--sp];
    }
    break;
  case 0x1000:
    // JP addr
    pc = (opcode & 0x0FFFu);
    break;
  case 0x2000:
    // CALL addr
    stack[sp++] = pc;
    pc = (opcode & 0x0FFFu);
    break;
  case 0x3000:
    // SE Vx, byte
    if (registers[(opcode & 0x0F00u) >> 8u] == (opcode & 0x00FFu)) {
      pc += 2;
    }
    break;
  case 0x4000:
    // SNE Vx, byte
    if (registers[(opcode & 0x0F00u) >> 8u] != (opcode & 0x00FFu)) {
      pc += 2;
    }

    break;

  case 0x5000:
    // SE Vx, Vy

    if (registers[(opcode & 0x0F00u) >> 8u] ==
        registers[(opcode & 0x00F0u) >> 4u]) {
      pc += 2;
    }

    break;

  case 0x6000:
    // LD Vx, byte
    registers[(opcode & 0x0F00u) >> 8u] = (opcode & 0x00FFu);
    break;
  case 0x7000:
    // ADD Vx, byte
    registers[(opcode & 0x0F00u) >> 8u] += (opcode & 0x00FFu);
    break;

  case 0x8000:
    opcode_0x8xyn(opcode);
    break;
  case 0x9000:
    // SNE Vx, Vy
    if (registers[(opcode & 0x0F00u) >> 8u] !=
        registers[(opcode & 0x00F0u) >> 4u]) {
      pc += 2;
    }
    break;
  case 0xA000:
    // LD I, addr
    index = (opcode & 0x0FFFu);
    break;
  case 0xB000:
    // JP V0, addr
    pc = registers[0] + (opcode & 0x0FFF);
    break;
  case 0xC000:
    // RND Vx, byte
    registers[(opcode & 0x0F00u) >> 8u] = dist(mt) & (opcode & 0x00FF);
    break;
  case 0xD000: {
    // DRW Vx, Vy, nibble
    std::uint8_t Vx = registers[(opcode & 0x0F00u) >> 8u] % VIDEO_WIDTH;
    std::uint8_t Vy = registers[(opcode & 0x00F0u) >> 4u] % VIDEO_HEIGHT;
    std::uint8_t height = opcode & 0x000Fu;

    registers[0xF] = 0;
    for (unsigned int yline = 0; yline < height; yline++) {
      std::uint8_t pixel = memory[index + yline];
      for (unsigned int xline = 0; xline < 8; xline++) {
        if ((pixel & (0x80 >> xline)) != 0) {
          const int isDisplayed =
              display[(Vx + xline + ((Vy + yline) * VIDEO_WIDTH))] ==
              COLOR_ENABLED;
          if (isDisplayed)
            registers[0xF] = 1;

          display[Vx + xline + ((Vy + yline) * VIDEO_WIDTH)] =
              1 ^ isDisplayed ? COLOR_ENABLED : COLOR_DISABLED;
        }
      }
    }

    draw = true;
  } break;
  case 0xE000: {
    const std::uint8_t Vx = registers[(opcode & 0x0F00u) >> 8u];
    switch (opcode & 0x00FFu) {
    case 0x009E:
      // Ex9E - SKP Vx
      if (keypad[Vx]) {
        pc += 2;
      }
      break;
    case 0x00A1:
      // SKNP Vx
      if (!keypad[Vx]) {
        pc += 2;
      }
      break;
    }
  } break;
  case 0xF000: {
    switch (opcode & 0x00FFu) {
    case 0x0007:
      // LD Vx, DT
      registers[(opcode & 0x0F00u) >> 8u] = delayTimer;
      break;
    case 0x000A: {
      // LD Vx, K
      bool wasKeyPressed = false;
      for (int i = 0; i < 16; i++) {
        if (keypad[i]) {
          wasKeyPressed = true;
          registers[(opcode & 0x0F00u) >> 8u] = i;
          break;
        }
      }

      if (!wasKeyPressed) {
        pc -= 2;
      }
      break;
    }
    case 0x0015:
      // LD DT, Vx
      delayTimer = registers[(opcode & 0x0F00u) >> 8u];
      break;
    case 0x0018:
      // LD ST, Vx
      soundTimer = registers[(opcode & 0x0F00u) >> 8u];
      break;
    case 0x001E:
      // ADD I, Vx
      index += registers[(opcode & 0x0F00u) >> 8u];
      break;
    case 0x0029: {
      std::uint8_t number = registers[(opcode & 0x0F00u) >> 8u];
      index = FONTSET_START_ADDRESS + (5 * number);
      break;
    }
    case 0x0033: {
      // LD B, Vx
      std::uint8_t num = registers[(opcode & 0x0F00u) >> 8u];
      for (int i = 2; i >= 0; i--) {
        memory[index + i] = num % 10;
        num /= 10;
      }
      break;
    }
    case 0x0055: {
      // LD [I], Vx
      std::uint8_t maxRegisters = (opcode & 0x0F00u) >> 8u;
      for (unsigned int i = 0; i <= maxRegisters; i++) {
        memory[index + i] = registers[i];
      }
      if (quirky) {
        index += maxRegisters + 1;
      }
      break;
    }
    case 0x0065: {
      // LD Vx, [I]
      std::uint8_t maxRegisters = (opcode & 0x0F00u) >> 8u;
      for (unsigned int i = 0; i <= maxRegisters; i++) {
        registers[i] = memory[index + i];
      }
      if (quirky) {
        index += maxRegisters + 1;
      }
      break;
    }
    default:
      std::cout << "Unknown op code: " << std::hex << opcode;
    }
    break;
  }
  default:
    std::cout << "Unknown op code: " << std::hex << opcode;
  }

  // Update timers
  if (delayTimer > 0)
    --delayTimer;

  if (soundTimer > 0) {
    if (soundTimer == 1)
      std::cout << "BEEP!\n";
    --soundTimer;
  }
}

void chip8::opcode_0x8xyn(std::uint16_t opcode) {
  switch (opcode & 0x000Fu) {
  case 0x0000:
    // LD Vx, Vy
    registers[(opcode & 0x0F00u) >> 8u] = registers[(opcode & 0x00F0u) >> 4u];
    break;
  case 0x0001:
    // OR Vx, Vy
    registers[(opcode & 0x0F00u) >> 8u] |= registers[(opcode & 0x00F0u) >> 4u];
    break;
  case 0x0002:
    // AND Vx, Vy
    registers[(opcode & 0x0F00u) >> 8u] &= registers[(opcode & 0x00F0u) >> 4u];
    break;
  case 0x0003:
    // XOR Vx, Vy
    registers[(opcode & 0x0F00u) >> 8u] ^= registers[(opcode & 0x00F0u) >> 4u];
    break;
  case 0x0004: {
    // ADD Vx, Vy
    // Set Vx = Vx + Vy, set VF = carry.
    std::uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    std::uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[0xF] =
        __builtin_add_overflow(registers[Vx], registers[Vy], &registers[Vx]);
    break;
  }
  case 0x0005: {
    // SUB Vx, Vy
    // Set Vx = Vx - Vy, set VF = NOT borrow.
    std::uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    std::uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[0xF] = registers[Vx] > registers[Vy];
    registers[Vx] -= registers[Vy];
    break;
  }
  case 0x0006: {
    // SHR Vx {, Vy}
    // Set Vx = Vx SHR 1.
    std::uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    if (quirky) {
      std::uint8_t Vy = (opcode & 0x00F0u) >> 4u;
      registers[0xF] = (registers[Vy] & 0x1u);
      registers[Vx] = registers[Vy] >> 1;
    } else {
      registers[0xF] = (registers[Vx] & 0x1u);
      registers[Vx] >>= 1;
    }
    break;
  }
  case 0x0007: {
    // SUBN Vx, Vy
    // Set Vx = Vy - Vx, set VF = NOT borrow.

    std::uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    std::uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[0xF] = registers[Vy] > registers[Vx];
    registers[Vx] = registers[Vy] - registers[Vx];
    break;
  }
  case 0x000E: {
    // SHL Vx {, Vy}
    // Set Vx = Vx SHL 1.

    std::uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    if (quirky) {
      std::uint8_t Vy = (opcode & 0x00F0u) >> 4u;
      registers[0xF] = (registers[Vy] & 0x80u);
      registers[Vx] = registers[Vy] << 1;
    } else {
      registers[0xF] = (registers[Vx] & 0x80) != 0;

      registers[Vx] <<= 1;
    }
    break;
  }
  default:
    std::cout << "Unknown op code: " << std::hex << opcode;
  }
}

sdlvideo::sdlvideo(const int scale = 10) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
    throw std::exception();
  }

  win = SDL_CreateWindow("Emulator", 100, 100, VIDEO_WIDTH * scale,
                         VIDEO_HEIGHT * scale, SDL_WINDOW_SHOWN);
  if (win == nullptr) {
    std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(win);
    SDL_Quit();
    throw std::exception();
  }

  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
  if (ren == nullptr) {
    SDL_DestroyWindow(win);
    std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    throw std::exception();
  }

  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  texture =
      SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, VIDEO_WIDTH, VIDEO_HEIGHT);
  if (texture == nullptr) {
    std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    throw std::exception();
  }

  SDL_RenderClear(ren);
}

sdlvideo::~sdlvideo() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

void sdlvideo::updateTexture(std::uint32_t *pixels) {
  SDL_RenderClear(ren);
  SDL_UpdateTexture(texture, nullptr, (void *)pixels,
                    VIDEO_WIDTH * sizeof(std::uint32_t));
  SDL_RenderCopy(ren, texture, nullptr, nullptr);
  SDL_RenderPresent(ren);
}

bool sdlvideo::setKeyStates(std::unordered_map<SDL_Keycode, int> keymap,
                            bool keys[16]) {

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      // exit if the window is closed
    case SDL_QUIT:
      return true;
    case SDL_KEYUP: {
      auto search = keymap.find(event.key.keysym.sym);
      if (search != keymap.end()) {
        // Key found
        keys[search->second] = false;
      }
    } break;
    case SDL_KEYDOWN: {
      auto search = keymap.find(event.key.keysym.sym);
      if (search != keymap.end()) {
        // Key found
        keys[search->second] = true;
      }
    } break;
    }
  }
  return false;
}

std::uint32_t convertStringToInt(std::string s) {
  std::uint32_t res;
  std::stringstream interpreter;
  interpreter << std::hex << s;
  interpreter >> res;
  return res;
}

int main(int argc, char **argv) {
  int DELAY = 1; // 1ms
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()("help,h", "produce help message")(
      "delay", po::value<int>(&DELAY)->default_value(1),
      "Set the program delay cycle in milliseconds")(
      "color-disabled,d", po::value<std::string>()->default_value("ff2d9bc8"),
      "Color set when pixels are disabled. In GBR")(
      "color-enabled,e", po::value<std::string>()->default_value("ff14dce6"),
      "Color set when pixels are disabled. In BGR")(
      "scale", po::value<int>()->default_value(20),
      "Scale the SDL window size by this amount. Default is 20.")(
      "input-file", po::value<std::string>(),
      "Input Chip-8 file to execute")("version,v", "print version string")(
      "quirky", po::bool_switch()->default_value(false),
      "Enable quirky mode, changes behaviour of 8xy6, 8xyE, Fx55, and Fx65 "
      "opcodes.");

  po::positional_options_description p;
  p.add("input-file", 1);
  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv).options(desc).positional(p).run(),
      vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: " << argv[0] << " [options] " << p.name_for_position(0)
              << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  if (vm.count("version")) {
    std::cout << "v0.1.0\n";
    return 0;
  }

  COLOR_ENABLED = convertStringToInt(vm["color-enabled"].as<std::string>());
  COLOR_DISABLED = convertStringToInt(vm["color-disabled"].as<std::string>());

  sdlvideo video(vm["scale"].as<int>());

  chip8 emulator(vm["input-file"].as<std::string>(), vm["quirky"].as<bool>());

  video.updateTexture(emulator.display);

  bool quit = false;

  while (!quit) {
    quit = video.setKeyStates(emulator.keymap, emulator.keypad);

    emulator.emulateCycle();
    if (emulator.draw) {
      video.updateTexture(emulator.display);
    }
    SDL_Delay(DELAY);
  }

  return 0;
}
