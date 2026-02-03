CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -g -I.
TARGET   = test_keyboard
SRC      = test_keyboard.cpp

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC) ../scancodes.h ../sanyombc-keyboard.ino
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
