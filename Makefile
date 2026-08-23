# crossctl build system.
#
# Profiles select compiler flags and an isolated object directory, so
# builds never collide:
#
#   make                 release build (-O2)
#   make PROFILE=debug   unoptimised with symbols
#   make test-asan       suite under AddressSanitizer
#   make test-ubsan      suite under UndefinedBehaviorSanitizer
#   make check           selftest + suite across all profiles

PROFILE ?= release

CC  := gcc
CXX := g++

WARN          := -Wall -Wextra -Werror
CFLAGS_BASE   := -std=c11   $(WARN) -Iinclude
CXXFLAGS_BASE := -std=c++17 $(WARN) -Iinclude
SAN_LDFLAGS   :=

ifeq ($(PROFILE),release)
  PROF_FLAGS := -O2 -DNDEBUG
endif
ifeq ($(PROFILE),debug)
  PROF_FLAGS := -O0 -g3
endif
ifeq ($(PROFILE),asan)
  PROF_FLAGS  := -O1 -g -fno-omit-frame-pointer -fsanitize=address
  SAN_LDFLAGS := -fsanitize=address
endif
ifeq ($(PROFILE),ubsan)
  PROF_FLAGS  := -O1 -g -fno-omit-frame-pointer \
                 -fsanitize=undefined -fno-sanitize-recover=all
  SAN_LDFLAGS := -fsanitize=undefined -fno-sanitize-recover=all
endif

ifndef PROF_FLAGS
  $(error unknown PROFILE '$(PROFILE)' — use release, debug, asan or ubsan)
endif

CFLAGS   := $(CFLAGS_BASE)   $(PROF_FLAGS)
CXXFLAGS := $(CXXFLAGS_BASE) $(PROF_FLAGS)

BUILD := build/$(PROFILE)
BIN   := $(BUILD)/crossctl

C_SRC   := $(wildcard clib/*.c)
CXX_SRC := $(wildcard src/*.cpp)
OBJ     := $(C_SRC:%.c=$(BUILD)/%.o) $(CXX_SRC:%.cpp=$(BUILD)/%.o)

.PHONY: all clean selftest test check \
        debug asan ubsan test-asan test-ubsan \
        docker-build docker-test docker-test-amd64 docker-shell release

all: $(BIN)
	@echo "built $(BIN) [profile: $(PROFILE)]"

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SAN_LDFLAGS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

selftest: $(BIN)
	./$(BIN) --selftest

test: $(BIN)
	@BIN=$(CURDIR)/$(BIN) tests/run.sh $(CASE)

debug:
	@$(MAKE) --no-print-directory PROFILE=debug all

asan:
	@$(MAKE) --no-print-directory PROFILE=asan all

ubsan:
	@$(MAKE) --no-print-directory PROFILE=ubsan all

test-asan:
	@$(MAKE) --no-print-directory PROFILE=asan selftest test

test-ubsan:
	@$(MAKE) --no-print-directory PROFILE=ubsan selftest test

# Everything, the way CI runs it.
check:
	@$(MAKE) --no-print-directory PROFILE=release selftest test
	@$(MAKE) --no-print-directory test-asan
	@$(MAKE) --no-print-directory test-ubsan
	@echo "=========================================="
	@echo "ALL PROFILES PASSED"

docker-build:
	docker build -f docker/Dockerfile -t crossctl:dev .

docker-test: docker-build
	docker run --rm crossctl:dev

docker-test-amd64:
	docker build --platform linux/amd64 -f docker/Dockerfile -t crossctl:amd64 .
	docker run --rm --platform linux/amd64 crossctl:amd64

docker-shell: docker-build
	docker run --rm -it crossctl:dev bash

release:
	@scripts/release.sh

clean:
	rm -rf build dist
