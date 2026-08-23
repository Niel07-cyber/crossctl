CC       := gcc
CXX      := g++
CFLAGS   := -std=c11   -Wall -Wextra -Werror -Iinclude
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Iinclude
BUILD    := build
BIN      := $(BUILD)/crossctl

C_SRC   := $(wildcard clib/*.c)
CXX_SRC := $(wildcard src/*.cpp)
OBJ     := $(C_SRC:%.c=$(BUILD)/%.o) $(CXX_SRC:%.cpp=$(BUILD)/%.o)

.PHONY: all clean selftest test

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

selftest: $(BIN)
	./$(BIN) --selftest

test: $(BIN)
	@tests/run.sh $(CASE)

clean:
	rm -rf $(BUILD)
