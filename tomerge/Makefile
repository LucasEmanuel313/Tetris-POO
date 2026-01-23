# Compiler and flags
CXX = g++
CXXFLAGS = -fdiagnostics-color=always -g -std=c++17
SFML_INCLUDE = -IC:\SFML-3.0.2\include
SFML_LIB = -LC:\SFML-3.0.2\lib
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Output executable
OUTPUT = Test_Graphic.exe

# Source files
SOURCES = Test_Graphic.cpp Button.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Targets
all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(SFML_LIB) $(SFML_LIBS) -o $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SFML_INCLUDE) -c $< -o $@

clean:
	del /Q *.o $(OUTPUT) 2>nul || true

run: $(OUTPUT)
	.\$(OUTPUT)

rebuild: clean all

.PHONY: all clean run rebuild
