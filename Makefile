CC       := gcc
CXX      := g++
CFLAGS   := -std=c11   -Wall -Wextra -Werror -Iinclude
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Iinclude
BUILD    := build
BIN      := $(BUILD)/crossctl

C_SRC   := $(wildcard clib/*.c)
CXX_SRC := $(wildcard src/*.cpp)
OBJ     := $(C_SRC:%.c=$(BUILD)/%.o) $(CXX_SRC:%.cpp=$(BUILD)/%.o)

.PHONY: all clean selftest test docker-build docker-test docker-test-amd64 docker-shell

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

docker-build:
	docker build -f docker/Dockerfile -t crossctl:dev .

docker-test: docker-build
	docker run --rm crossctl:dev

docker-test-amd64:
	docker build --platform linux/amd64 -f docker/Dockerfile -t crossctl:amd64 .
	docker run --rm --platform linux/amd64 crossctl:amd64

docker-shell: docker-build
	docker run --rm -it crossctl:dev bash

clean:
	rm -rf $(BUILD)
