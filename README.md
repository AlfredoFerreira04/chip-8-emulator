#  Chip-8 Emulator

An emulator/interpreter for the Chip-8 programming language, written in C++. Built as a fun learning project to explore low-level emulation as a hobby!

This project was built with help from "Cowgod's Chip-8 Technical Reference v1.0" (made by Cowgod himself) and Tobias Langhoff's "Guide to making a CHIP-8 emulator", I primarly used Cowgod's documentation but used Tobias' article whenever I was unsure about my interpretation of an instruction so I could double check.

[Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#5xy0)

[Guide to making a CHIP-8 emulator by Tobias Langhoff](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)

The project was tested using the files made available by community member [Timendus](https://github.com/Timendus), a great thanks to him aswell for his contribution to the space.

---

##  Features

- Full instruction set implementation (All Chip-8 opcodes)
- Keyboard input support
- Compatible with standard test ROMs
- Debug build with GDB support


## Prerequisites

- C++17 or newer
- [SDL2](https://www.libsdl.org/) installed
- `make` (for using the Makefile)
- Linux/macOS (or WSL on Windows)



## Keyboard Mapping

### Keyboard → CHIP-8

|1|2|3|4| → |1|2|3|C|

|Q|W|E|R| → |4|5|6|D|

|A|S|D|F| → |7|8|9|E|

|Z|X|C|V| → |A|0|B|F|


## Build Instructions

This project uses `g++` and SDL2. Make sure both are installed and available in your system's environment.

### Prerequisites

- A C++17-compatible compiler (e.g., `g++`)
- [SDL2](https://www.libsdl.org/) development libraries
- `make` utility

#### On Linux:

```sh
sudo apt-get install build-essential libsdl2-dev
```

#### Building the Emulator

To build the release version of the emulator:

```sh
make
```

#### This will produce an executable named:

```sh
chip8_emulator
```

##### To build the debug version with debugging symbols:

```sh
make debug
```

#### This will produce:

```sh
chip8_debug
```


#### To run the release build:

```sh
make run
```

#### To launch the debug build in GDB:

```sh
make gdb
```

#### To remove all compiled binaries and object files:

```sh
make clean
```
