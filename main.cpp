#include "chip8.h"
#include "chip8.cpp"

int main() {
    Chip8 chip8; // emulator instance

    std::fstream inputStream("assets/ROMS/ibm.ch8", std::ios::in | std::ios::binary | std::ios::ate);

    if (!inputStream.is_open()) {
        std::cout << "Problem reading file\n";
        return 1;
    }

    chip8.romSize  = inputStream.tellg();    
    inputStream.seekg(0, std::ios::beg);

    if (chip8.romSize > 0 && chip8.romSize <= (4096 - 0x200)) {
        inputStream.read(reinterpret_cast<char*>(&chip8.memory[0x200]), chip8.romSize);
    } else {
        std::cout << "ROM too big or empty\n";
        inputStream.close();
        return 1;
    }

    inputStream.close();
    std::cout << "ROM loaded into memory at 0x200\n";


    
    while (chip8.hasMoreOpcodes()) {
        chip8.decodeNextOpCode();
    }
    


    return 0;
}
