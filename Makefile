# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`
DEBUGFLAGS = -g

# Output binary names
TARGET = chip8_emulator
DEBUG_TARGET = chip8_debug

# Source files
SRCS = main.cpp chip8.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)
DEBUG_OBJS = $(SRCS:.cpp=.debug.o)

# Default rule: build the emulator
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Rule to compile .cpp files to .debug.o files
%.debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

# Debug build
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ $^ $(LDFLAGS)

# Clean rule to delete compiled files
clean:
	rm -f $(OBJS) $(DEBUG_OBJS) $(TARGET) $(DEBUG_TARGET)

# Run the release binary
run: $(TARGET)
	./$(TARGET)

# Run the debug binary in GDB
gdb: debug
	gdb ./$(DEBUG_TARGET)

.PHONY: all clean run debug gdb
