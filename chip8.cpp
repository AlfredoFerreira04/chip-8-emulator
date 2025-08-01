#include "chip8.h"



class Chip8 {
    public:
        uint8_t memory[4096] = {0};           // Memory for the Chip-8 system

        uint8_t registers[16] = {0};          // 16 registers (V0 to VF, hexadecimal)

        uint8_t delay_timer;            // Delay timer
        uint8_t sound_timer;            // Sound timer

        uint16_t I;                     // Index register, 16-bit register for memory addresses usually only uses 12 bits because of that
        uint16_t pc = 0x200;            // Program counter
        size_t romSize = 0;             // Rom Size

        uint8_t sp;                     // Stack pointer
        uint16_t stack;                 // Stack for storing return addresses
        
        uint16_t keypad;                // Keypad state (0-15)
        uint8_t gfx[64 * 32] = {0};     // Graphics memory (64x32 pixels)


        
        
        uint16_t readNextOpCode() {
            if (!hasMoreOpcodes()) {
                // Could throw, return 0, or handle gracefully
                return 0;
            }
            uint16_t opCode = (memory[pc] << 8) | memory[pc + 1];
            pc += 2;
            return opCode;
        }
        
        // Read next OpCode and increment program counter
        uint16_t fetchNextOpCode() {
            if (!hasMoreOpcodes()) {
                return 0;
            }
            uint16_t opCode = (memory[pc] << 8) | memory[pc + 1];
            pc += 2;
            return opCode;
        }

        void decodeNextOpCode() {
            uint16_t opcode = fetchNextOpCode();

            if (opcode == 0) {
                return;  // no more opcodes or end
            }

            // Extract nibbles and bytes for decoding
            uint8_t nibble1 = (opcode & 0xF000) >> 12;
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t Y = (opcode & 0x00F0) >> 4;
            uint8_t N = opcode & 0x000F;
            uint8_t NN = opcode & 0x00FF;
            uint16_t NNN = opcode & 0x0FFF;

            switch (nibble1) {
                case 0x0:
                    if (opcode == 0x00E0) {
                        clearScreen();
                    }
                    // Could add 0x00EE (return) here
                    break;

                case 0x1:
                    jump(NNN);
                    break;

                case 0x6:
                    setRegisterVc(X, NN);
                    break;

                case 0x7:
                    addToRegister(X, NN);
                    break;

                case 0xA:
                    setIndexRegister(NNN);
                    break;

                case 0xD:
                    drawOnScreen(X, Y, N);
                    break;

                // Add other opcode cases here...

                default:
                    std::cout << "Unknown opcode: 0x" << std::hex << opcode << "\n";
                    break;
            }
        }



        bool hasMoreOpcodes() {
            return pc < (0x200 + romSize);
        }
        
        // Processes OpCode '00E0', which clears the screen or display buffer
        void clearScreen(){
            std::fill_n(gfx, 64*32, 0);
        }

        // Processes OpCode '1NNN', which sets the program counter to value 'NNN'
        void jump(uint16_t newPC) {
            pc = newPC;
        }

        // Processes OpCode '6XNN', which sets register 'X' to value  'NN'
        void setRegisterVc(uint8_t reg, uint8_t value){
            registers[reg] = value;
        }

        // Processes OpCode '7XNN', which adds value 'NN' to value in register 'X'
        void addToRegister(uint8_t reg, uint8_t value){
            registers[reg] += value;
        }

        // Processes OpCode 'ANNN', which sets Index (or 'I') register value to 'NNN'
        void setIndexRegister(uint16_t value){
            I = value;
        }
        
        // Processes OpCode 'DXYN', this operation draws on the screen/display and is the most complicated expression
        // 'X' refers to the value of register Vx, which will contain the coordinate X from which to start drawing
        // 'Y' refers to the value of register Vy, which will contain the coordinate Y from which to start drawing
        // 'N' refers to the number of rows that will be drawn in this operation
        // The operation will go to the memory location pointed to by the Index Register (I) and grab N bytes from it
        // after this it will draw N rows of 8 pixels (1-byte = 8-bits, 1-bit = pixel) by XOR'ing the bit's corresponding
        // to each pixel, if a bit is XOR'd to 0 (1XOR1) VF (V15 or register 15) will be set to 1, otherwise it is set to 0
        void drawOnScreen(uint8_t vx, uint8_t vy, uint8_t n){
            registers[0xF] = 0;

            // get coords from registers
            uint8_t x = registers[vx];
            uint8_t y = registers[vy];

            // initiate spriteBuffer with rows of pixels for sprite
            uint8_t spriteBuffer[n];
            std::copy(&memory[I], &memory[I]+n, spriteBuffer);

            for(int i=0; i<n; i++){
                uint8_t xCoord = x;
                uint8_t yCoord = y + i;
                
                // check if the sprite goes over vertical edge of the screen, if so it wraps around
                if(yCoord >= 32) { yCoord = 0; }

                std::bitset<8> bits(spriteBuffer[i]);   // sprite byte 'n' split into bits to XOR with screen bits

                for(int j=0; j<8; j++){

                    bool pixelWasOn = true;
                    xCoord = x + j;
                    
                    // check if the sprite goes over horizontal edge of the screen, if so it wraps around
                    if(xCoord >= 64) { xCoord = 0; }

                    int index = yCoord * 64 + xCoord;
                    
                    if (gfx[index] == 0) {pixelWasOn = false;}

                    gfx[index] = ((uint8_t)bits[7 - j] ^ gfx[index]);

                    if (gfx[index] == 0 && pixelWasOn == true){ registers[0xF] = 1;}

                }
                displayScreen();
            }

            
        }

        void displayScreen() {
            for (int y = 0; y < 32; y++) {
                for (int x = 0; x < 64; x++) {
                    int index = y * 64 + x;
                    if (gfx[index] == 1) {
                        std::cout << "x";
                    } else {
                        std::cout << " ";
                    }
                }
                std::cout << "\n";
            }
        }

        
};