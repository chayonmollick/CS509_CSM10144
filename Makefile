# CS509 - builds the common wrapper (bin/cs509) and the test-input generators (bin/gen_*).
CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -pedantic
ALL_CXXFLAGS := -std=c++17 $(CXXFLAGS)
CPPFLAGS += -Icommon/include -Iassignment3/include -Iassignment4/include

BUILD := build
BIN   := bin

COMMON_SRC := $(wildcard common/src/*.cpp)
APP_SRC    := driver/main.cpp $(wildcard assignment3/src/*.cpp) $(wildcard assignment4/src/*.cpp)
TOOLS      := $(patsubst tools/%.cpp,$(BIN)/%,$(wildcard tools/*.cpp))

COMMON_LIB := $(BUILD)/libcs509_common.a

.PHONY: all tools clean

all: $(BIN)/cs509 tools

tools: $(TOOLS)

# Code from Assignments 1-2 (input scanner, adjacency-list reader, CSR conversion) is built once
# as a library and linked into the wrapper, so later assignments call it instead of copying it.
$(COMMON_LIB): $(COMMON_SRC:%.cpp=$(BUILD)/%.o)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

$(BIN)/cs509: $(APP_SRC:%.cpp=$(BUILD)/%.o) $(COMMON_LIB)
	@mkdir -p $(@D)
	$(CXX) $(ALL_CXXFLAGS) $^ -o $@

$(BIN)/%: tools/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(ALL_CXXFLAGS) $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(ALL_CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD) $(BIN)

-include $(wildcard $(BUILD)/*/*.d $(BUILD)/*/*/*.d)
