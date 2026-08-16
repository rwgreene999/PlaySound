CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
DEBUG_CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -pedantic
TARGET := PlaySound
SRC := PlaySound.cpp

.PHONY: all debug run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

debug: $(SRC)
	$(CXX) $(DEBUG_CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
