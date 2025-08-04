#include "chip8.h"
#include "chip8.cpp"

int main(int argc, char *argv[]) {
    Chip8 chip8; // emulator instance
    
    // Load font set into memory (at location 0x000 to 0x050)
    // Each character is 5 bytes
    // This is not very elegant but this instruction is so boring to code I just want to get it over with
    uint8_t fontset[80] = {
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
    
    // Load fontset into memory
    for (int i = 0; i < 80; ++i) {
        chip8.memory[i] = fontset[i];
    }
    
    // Initialize SDL
    if (!chip8.initializeSDL()) {
        std::cerr << "Failed to initialize SDL. Exiting..." << std::endl;
        return 1;
    }

    // Determine ROM file to load
    std::string romPath = "assets/ROMS/5-quirks.ch8"; // Default ROM
    
    // If a ROM file is specified as a command line argument, use it instead
    if (argc > 1) {
        romPath = argv[1];
    }
    
    std::cout << "Loading ROM: " << romPath << std::endl;
    
    // Load the ROM file
    std::fstream inputStream(romPath, std::ios::in | std::ios::binary | std::ios::ate);

    if (!inputStream.is_open()) {
        std::cout << "Problem reading file\n";
        chip8.cleanupSDL();
        return 1;
    }

    chip8.romSize = inputStream.tellg();    
    inputStream.seekg(0, std::ios::beg);

    if (chip8.romSize > 0 && chip8.romSize <= (4096 - 0x200)) {
        inputStream.read(reinterpret_cast<char*>(&chip8.memory[0x200]), chip8.romSize);
    } else {
        std::cout << "ROM too big or empty\n";
        inputStream.close();
        chip8.cleanupSDL();
        return 1;
    }

    inputStream.close();
    std::cout << "ROM loaded into memory at 0x200\n";


    
    // --- Timing setup ---
    using clock = std::chrono::high_resolution_clock;
    auto lastTimerTick = clock::now();
    auto lastCycleTick = clock::now();
    const std::chrono::duration<double> timerInterval(1.0 / 60.0);    // 60Hz for timers
    const std::chrono::duration<double> cycleInterval(1.0 / 500.0);   // CPU speed (~500Hz)

    // --- Main loop ---
    bool running = true;
    while (running && chip8.hasMoreOpcodes()) {
        // Process user input
        chip8.handleInput();
        
        // Execute CPU cycles at the target rate
        auto currentTime = clock::now();
        if (currentTime - lastCycleTick >= cycleInterval) {
            chip8.decodeNextOpCode();
            lastCycleTick = currentTime;
        }

        // Handle 60Hz ticking of delay and sound timers
        if (currentTime - lastTimerTick >= timerInterval) {
            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) {
                if (chip8.sound_timer == 1) {
                    // Beep sound would go here
                    std::cout << "BEEP!" << std::endl;
                }
                chip8.sound_timer--;
            }
            lastTimerTick = currentTime;
        }
        
        // Limit the frame rate
        SDL_Delay(1);
    }

    // Clean up SDL resources before exit
    chip8.cleanupSDL();
    return 0;
}

