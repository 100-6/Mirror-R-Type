# Makefile for SFML and Raylib POCs

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# SFML
SFML_SRC = poc_sfml.cpp
SFML_TARGET = poc_sfml
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Raylib
RAYLIB_SRC = poc_raylib.cpp
RAYLIB_TARGET = poc_raylib
RAYLIB_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Targets
.PHONY: all clean sfml raylib run-sfml run-raylib

all: sfml raylib

# Compile SFML POC
sfml: $(SFML_TARGET)

$(SFML_TARGET): $(SFML_SRC)
	@echo "Compiling SFML POC..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(SFML_LIBS)
	@echo "✓ SFML POC compiled successfully!"

# Compile Raylib POC
raylib: $(RAYLIB_TARGET)

$(RAYLIB_TARGET): $(RAYLIB_SRC)
	@echo "Compiling Raylib POC..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(RAYLIB_LIBS)
	@echo "✓ Raylib POC compiled successfully!"

# Run SFML POC
run-sfml: $(SFML_TARGET)
	@echo "Running SFML POC..."
	./$(SFML_TARGET)

# Run Raylib POC
run-raylib: $(RAYLIB_TARGET)
	@echo "Running Raylib POC..."
	./$(RAYLIB_TARGET)

# Clean compiled files
clean:
	@echo "Cleaning..."
	rm -f $(SFML_TARGET) $(RAYLIB_TARGET)
	@echo "✓ Clean complete!"

# Help
help:
	@echo "Available targets:"
	@echo "  make all         - Compile both SFML and Raylib POCs"
	@echo "  make sfml        - Compile SFML POC only"
	@echo "  make raylib      - Compile Raylib POC only"
	@echo "  make run-sfml    - Compile and run SFML POC"
	@echo "  make run-raylib  - Compile and run Raylib POC"
	@echo "  make clean       - Remove compiled binaries"
	@echo "  make help        - Show this help message"
