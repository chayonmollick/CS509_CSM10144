CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = simulator
SRCS = main.cpp compiler.cpp processor.cpp memory.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) program.byte data.out

run: $(TARGET)
	./$(TARGET) examples/sum4.txt
