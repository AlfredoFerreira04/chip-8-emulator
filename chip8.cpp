#include "chip8.h"

class Chip8 {
    public:
        // Memory and registers
        uint8_t memory[4096] = {0};           // Memory for the Chip-8 system
        uint8_t registers[16] = {0};          // 16 registers (V0 to VF, hexadecimal)
        uint8_t delay_timer;                  // Delay timer
        uint8_t sound_timer;                  // Sound timer
        uint16_t I;                           // Index register, 16-bit register for memory addresses usually only uses 12 bits because of that
        uint16_t pc = 0x200;                  // Program counter
        size_t romSize = 0;                   // Rom Size
        uint8_t sp = 0;                       // Stack pointer
        uint16_t stack[16];                   // Stack for storing return addresses
        uint8_t keypad[16] = {0};             // Keypad state (0-15), array of 16 keys
        uint8_t pressedKey = -1;
        uint8_t gfx[64 * 32] = {0};           // Graphics memory (64x32 pixels)
        
        // SDL-specific members
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;
        uint32_t pixels[64 * 32] = {0};       // RGBA pixel buffer for rendering
        
        // Constants for rendering
        const int PIXEL_SIZE = 10;            // Size of each CHIP-8 pixel
        const uint32_t ON_COLOR = 0xFFFFFFFF; // White color for ON pixels
        const uint32_t OFF_COLOR = 0x00000000; // Black color for OFF pixels

        std::mutex mux;





        
        
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

        // This method is huge, but I can't be bothered to do something more 'optimal' for a pet project
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
                } else if (opcode == 0x00EE) {
                    returnFromSubroutine();
                }
                break;

            case 0x1:
                jump(NNN);
                break;
                
            case 0x2:
                callSubroutine(NNN);
                break;
                
            case 0x3:
                skipNextInstructionValueEq(X, NN);
                break;
                
            case 0x4:
                skipNextInstructionValueDiff(X, NN);
                break;
                
            case 0x5:
                if (N == 0) {
                    skipNextInstructionRgister(X, Y);
                }
                break;

            case 0x6:
                setRegisterVc(X, NN);
                break;

            case 0x7:
                addToRegister(X, NN);
                break;
                
            case 0x8:
                switch (N) {
                    case 0x0:
                        copyRegister(X, Y);
                        break;
                    case 0x1:
                        bitwiseOR(X, Y);
                        break;
                    case 0x2:
                        bitwiseAND(X, Y);
                        break;
                    case 0x3:
                        bitwiseXOR(X, Y);
                        break;
                    case 0x4:
                        registersADD(X, Y);
                        break;
                    case 0x5:
                        registersSUB(X, Y);
                        break;
                    case 0x6:
                        registersSHR(X, Y);
                        break;
                    case 0x7:
                        registersSUBN(X, Y);
                        break;
                    case 0xE:
                        registersSHL(X, Y);
                        break;
                }
                break;
                
            case 0x9:
                if (N == 0) {
                    skipNextInstruction(X, Y);
                }
                break;

            case 0xA:
                setIndexRegister(NNN);
                break;
                
            case 0xB:
                jumpWithV0(NNN);
                break;
                
            case 0xC:
                randomByteAnd(X, NN);
                break;

            case 0xD:
                drawOnScreen(X, Y, N);
                break;
                
            case 0xE:
                if (NN == 0x9E) {
                    skipNextInstructionIfKeyPressed(X);
                } else if (NN == 0xA1) {
                    skipNextInstructionIfKeyNotPressed(X);
                }
                break;
                
            case 0xF:
                switch (NN) {
                    case 0x07:
                        storeDelayTimer(X);
                        break;
                    case 0x0A:
                        waitForKeyPress(X);
                        break;
                    case 0x15:
                        setDelayTimer(X);
                        break;
                    case 0x18:
                        setSoundTimer(X);
                        break;
                    case 0x1E:
                        updateIndex(X);
                        break;
                    case 0x29:
                        setIToDigitSprite(X);
                        break;
                    case 0x33:
                        storeBCDRepresentation(X);
                        break;
                    case 0x55:
                        assignToMemory(X);
                        break;
                    case 0x65:
                        assignToRegisters(X);
                        break;
                }
                break;

            default:
                std::cout << "Unknown opcode: 0x" << std::hex << opcode << "\n";
                break;
        }
}

        bool hasMoreOpcodes() {
            return pc < (0x200 + romSize);
        }

        // Processes OpCode '00E0', which clears the screen or display buffer
        void clearScreen() {
            std::fill_n(gfx, 64*32, 0);
            // Update the display after clearing
            displayScreen();
        }

        // Processes OpCode '00EE', which returns from a subroutine by decrementing the stack once
        void returnFromSubroutine(){
            pc = stack[sp];
            sp--;
        }

        // Processes OpCode '1NNN', which sets the program counter to value 'NNN'
        void jump(uint16_t newPC) {
            pc = newPC;
        }

        // Processes OpCode '2NNN', which calls a new subroutine and sets the top of the stack to the current program counter
        void callSubroutine(uint16_t newSubroutine){
            sp++;
            stack[sp] = pc;

            pc = newSubroutine;
        }

        // Processes OpCode '3XNN', which skips the next OpCode if register X holds value equal to NN
        void skipNextInstructionValueEq(uint8_t x, uint8_t value){
            if(registers[x] == value){
                pc +=2;
            }
        }

        // Processes OpCode '4XNN', which skips the next OpCode if register X holds value different to NN
        void skipNextInstructionValueDiff(uint8_t x, uint8_t value){
            if(registers[x] != value){
                pc +=2;
            }
        }

        // Processes OpCode '5XY0', which skips the next OpCode if register X holds value is equal to register Y
        void skipNextInstructionRgister(uint8_t x, uint8_t y){
            if(registers[x] == registers[y]){
                pc +=2;
            }
        }

        // Processes OpCode '6XNN', which sets register 'X' to value  'NN'
        void setRegisterVc(uint8_t reg, uint8_t value){
            registers[reg] = value;
        }

        // Processes OpCode '7XNN', which adds value 'NN' to value in register 'X'
        void addToRegister(uint8_t reg, uint8_t value){
            registers[reg] += value;
        }

        // Processes OpCode '8XY0', which sets the value in register 'X' to be equal to the value in register 'Y'
        void copyRegister(uint8_t x, uint8_t y){
            registers[x] = registers[y];
        }

        // Processes OpCode '8XY1', performs an OR operation on the values of Vx and Vy and stores the value on Vx
        void bitwiseOR(uint8_t x, uint8_t y){
            registers[x] = registers[x] | registers[y];
        }

        // Processes OpCode '8XY2', performs an AND operation on the values of Vx and Vy and stores the value on Vx
        void bitwiseAND(uint8_t x, uint8_t y){
            registers[x] = registers[x] & registers[y];
        }

        // Processes OpCode '8XY3', performs an AND operation on the values of Vx and Vy and stores the value on Vx
        void bitwiseXOR(uint8_t x, uint8_t y){
            registers[x] = registers[x] ^ registers[y];
        }

        // Processes OpCode '8XY4', Vx = Vx + Vy, if Vx > 255 then VF is set to 1 and the lowest 8 bits of the result are stored in Vx
        void registersADD(uint8_t x, uint8_t y){
            if(255 < (registers[x] + registers[y])){
                registers[x] = (registers[x] + registers[y]) - 255;
                registers[0xF] = 1;
            }else{
                registers[x] = (registers[x] + registers[y]);
                registers[0xF] = 0;
            }
        }

        // Processes OpCode '8XY5', Vx = Vx - Vy, if Vx > Vy then VF is set to 1 otherwise 0
        void registersSUB(uint8_t x, uint8_t y){
            if(registers[x] > registers[y]){
                registers[x] = (registers[x] - registers[y]) ;
                registers[0xF] = 1;
            }else{
                registers[x] = (registers[x] - registers[y]);
                registers[0xF] = 0;
            }
        }

        // Processes OpCode '8XY6', which divides Vx by 2 (right bitshift of 1), if Vx is odd it sets VF to 1
        // Note: this instruction includes register Vy as it is believed that originally this instruction meant to store in Vx the value of Vy bitshifted
        // but since it was undocumented in the originaly system specifications it was mostly reverse engineered and Vy seems to be not be used.
        void registersSHR(uint8_t x, uint8_t y){
            registers[0xF] = (registers[x] << 7);
            registers[0xF] = registers[0xF] >> 7;
            registers[x] = registers[x] >> 1;
        }

        // Processes OpCode '8XY7', Vx = Vy - Vx, if Vy > Vx then VF is set to 1 otherwise 0
        void registersSUBN(uint8_t x, uint8_t y){
            if(registers[y] > registers[x]){
                registers[x] = (registers[y] - registers[x]) ;
                registers[0xF] = 1;
            }else{
                registers[x] = (registers[y] - registers[x]);
                registers[0xF] = 0;
            }
        }

        // Processes OpCode '8XYE', which multiplies Vx by 2 (left bitshift of 1), if Vx >= 128 (10000000) it sets VF to 1
        // Note: this instruction includes register Vy as it is believed that originally this instruction meant to store in Vx the value of Vy bitshifted
        // but since it was undocumented in the originaly system specifications it was mostly reverse engineered and Vy seems to be not be used.
        void registersSHL(uint8_t x, uint8_t y){
            registers[0xF] = registers[x] >> 7;
            registers[x] = registers[x] << 1;
        }

        // Processes OpCode '9XY0', which skips the next instruction of the values of Vx and Vy differ.
        void skipNextInstruction(uint8_t x, uint8_t y){
            if(registers[x] != registers[y]){
                pc +=2;
            }
        }

        // Processes OpCode 'ANNN', which sets Index (or 'I') register value to 'NNN'
        void setIndexRegister(uint16_t value){
            I = value;
        }

        // Processes OpCode 'BNNN', which sets pc to NNN + V0, basically a more complex version of the jump command, I assume for bigger jumps
        void jumpWithV0(uint16_t value){
            pc = registers[0] + value;
        }

        // Processes OpCode 'CXNN', which sets Vx value to the result of the AND operation between a random byte and NN.
        void randomByteAnd(uint8_t x, uint8_t value){
            uint8_t randomByte = rand();

            registers[x] = randomByte & value;
            
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
                
            }
            displayScreen();
            
        }

        // Processed OpCode 'Ex9E', which skips the next OpCode if key in Vx is pressed
        void skipNextInstructionIfKeyPressed(uint8_t x){
            if(keypad[registers[x]] == 1){
                pc +=2;
            }
        }

        // Processed OpCode 'ExA1', which skips the next OpCode if key in Vx is NOT pressed
        void skipNextInstructionIfKeyNotPressed(uint8_t x){
            if(keypad[registers[x]] == 0){
                pc +=2;
            }
        }

        // Processed Opcode 'Fx07', which places the current delay timer value in register Vx
        void storeDelayTimer(uint8_t x){
            registers[x] = delay_timer;
        }

        // Processes OpCode 'Fx0A', which wait for a key press, store the value of the key in Vx
        void waitForKeyPress(uint8_t x){
            if(pressedKey == -1){
                pc -=2;
            }else{
                registers[x] = pressedKey;
                pressedKey = -1;
            }
        }

        // Processes OpCode 'Fx15', which updates the value of the delay timer with the value of register Vx
        void setDelayTimer(uint8_t x){
            delay_timer = registers[x];
        }

        // Processes OpCode 'Fx18', which updates the value of the sound timer with the value of register Vx
        void setSoundTimer(uint8_t x){
            sound_timer = registers[x];
        }

        // Processes OpCode 'Fx1E', which adds the value in register Vx to the index register I
        void updateIndex(uint8_t x){
            I += registers[x];
        }

        // Processes OpCode 'Fx29', which sets I to the location of sprite for digit Vx
        void setIToDigitSprite(uint8_t x){
            // Each font character is 5 bytes tall and stored starting at memory address 0
            // The font data is typically loaded at the beginning of memory
            uint8_t digit = registers[x] & 0x0F; // Get only the lowest 4 bits (0-F)
            I = digit * 5; // Each digit sprite is 5 bytes tall
        }

        // Processes OpCode 'Fx33', which stores the binary-coded decimal representation
        // of the value in register Vx at memory locations I, I+1, and I+2.
        // For example, if Vx contains 156:
        // - memory[I] = 1 (hundreds digit)
        // - memory[I+1] = 5 (tens digit)
        // - memory[I+2] = 6 (ones digit)
        void storeBCDRepresentation(uint8_t x){
            uint8_t value = registers[x];
            memory[I] = value / 100;                // Hundreds digit
            memory[I + 1] = (value / 10) % 10;      // Tens digit
            memory[I + 2] = value % 10;             // Ones digit
        }

        // Processes OpCode 'Fx55', which stores the values of registers V0->Vx into memory starting from I
        void assignToMemory(uint8_t x){
            for(int j = 0; j<=x; j++){
                memory[I+j] = registers[j];
            }
        }

        // Processes OpCode 'Fx65', which stores the values of registers V0->Vx into memory starting from I
        void assignToRegisters(uint8_t x){
            for(int j = 0; j<=x; j++){
                registers[j] = memory[I+j];
            }
        }

        // Initialize SDL systems
        bool initializeSDL() {
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
                std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            window = SDL_CreateWindow("CHIP-8 Emulator", 
                                      SDL_WINDOWPOS_CENTERED, 
                                      SDL_WINDOWPOS_CENTERED,
                                      64 * PIXEL_SIZE, 
                                      32 * PIXEL_SIZE, 
                                      SDL_WINDOW_SHOWN);
            if (!window) {
                std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer) {
                std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            texture = SDL_CreateTexture(renderer, 
                                        SDL_PIXELFORMAT_RGBA8888, 
                                        SDL_TEXTUREACCESS_STREAMING, 
                                        64, 32);
            if (!texture) {
                std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            // Initialize pixels to OFF_COLOR
            for (int i = 0; i < 64 * 32; i++) {
                pixels[i] = OFF_COLOR;
            }
            
            return true;
        }
        
        // Cleanup SDL resources
        void cleanupSDL() {
            if (texture) {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }
            if (renderer) {
                SDL_DestroyRenderer(renderer);
                renderer = nullptr;
            }
            if (window) {
                SDL_DestroyWindow(window);
                window = nullptr;
            }
            SDL_Quit();
        }
        
        // Handle SDL events and input
        void handleInput() {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    exit(0); 
                } else if (event.type == SDL_KEYDOWN) {

                    // Map keyboard keys to CHIP-8 keypad according to requested layout:
                    // Keyboard:   CHIP-8 hex value mapping:
                    // 1 2 3 4     1 2 3 C
                    // q w e r     4 5 6 D
                    // a s d f     7 8 9 E
                    // z x c v     A 0 B F
                    switch (event.key.keysym.sym) {
                        case SDLK_1: keypad[0x1] = 1; break; // 1
                        case SDLK_2: keypad[0x2] = 1; break; // 2
                        case SDLK_3: keypad[0x3] = 1; break; // 3
                        case SDLK_4: keypad[0xC] = 1; break; // C
                        
                        case SDLK_q: keypad[0x4] = 1; break; // 4
                        case SDLK_w: keypad[0x5] = 1; break; // 5
                        case SDLK_e: keypad[0x6] = 1; break; // 6
                        case SDLK_r: keypad[0xD] = 1; break; // D
                        
                        case SDLK_a: keypad[0x7] = 1; break; // 7
                        case SDLK_s: keypad[0x8] = 1; break; // 8
                        case SDLK_d: keypad[0x9] = 1; break; // 9
                        case SDLK_f: keypad[0xE] = 1; break; // E
                        
                        case SDLK_z: keypad[0xA] = 1; break; // A
                        case SDLK_x: keypad[0x0] = 1; break; // 0
                        case SDLK_c: keypad[0xB] = 1; break; // B
                        case SDLK_v: keypad[0xF] = 1; break; // F
                        
                        case SDLK_ESCAPE: exit(0); break;    // ESC to quit
                    }
                } else if (event.type == SDL_KEYUP) {
                    switch (event.key.keysym.sym) {
                        case SDLK_1: keypad[0x1] = 0; break;
                        case SDLK_2: keypad[0x2] = 0; break;
                        case SDLK_3: keypad[0x3] = 0; break;
                        case SDLK_4: keypad[0xC] = 0; break;
                        
                        case SDLK_q: keypad[0x4] = 0; break;
                        case SDLK_w: keypad[0x5] = 0; break;
                        case SDLK_e: keypad[0x6] = 0; break;
                        case SDLK_r: keypad[0xD] = 0; break;
                        
                        case SDLK_a: keypad[0x7] = 0; break;
                        case SDLK_s: keypad[0x8] = 0; break;
                        case SDLK_d: keypad[0x9] = 0; break;
                        case SDLK_f: keypad[0xE] = 0; break;
                        
                        case SDLK_z: keypad[0xA] = 0; break;
                        case SDLK_x: keypad[0x0] = 0; break;
                        case SDLK_c: keypad[0xB] = 0; break;
                        case SDLK_v: keypad[0xF] = 0; break;
                    }
                }
            }
        }

        // Update the screen with SDL
        void displayScreen() {
            // Update pixel buffer from gfx array
            for (int i = 0; i < 64 * 32; i++) {
                pixels[i] = gfx[i] ? ON_COLOR : OFF_COLOR;
            }
            
            // Update texture with new pixel data
            SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
            
            // Clear renderer and render the texture
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            
            // Handle events
            handleInput();
        }

        
};