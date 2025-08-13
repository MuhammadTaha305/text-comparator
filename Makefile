# Simple Text Analyzer Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lstdc++fs
TARGET = text_analyzer
SOURCES = main.cpp simple_analyzer.cpp
HEADERS = simple_analyzer.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Clean build files
clean:
	rm -f $(TARGET) *.o

# Run the program
run: $(TARGET)
	./$(TARGET)

# Install dependencies (for Windows, you might need to install MinGW)
install-deps:
	@echo "For Windows, install MinGW-w64 or use Visual Studio"
	@echo "For Linux/Mac: sudo apt-get install g++ (Ubuntu) or brew install gcc (Mac)"

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build the executable"
	@echo "  clean      - Remove build files"
	@echo "  run        - Build and run the program"
	@echo "  install-deps - Show dependency installation info"
	@echo "  help       - Show this help message"

.PHONY: all clean run install-deps help
