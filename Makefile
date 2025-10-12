################################################################################
# Project: TP2 - Sistema de Backup (C++)
# Tooling: cpplint, cppcheck, valgrind, gcov/lcov, doxygen, Catch2 (single-header)
################################################################################

# Directories
SRC_DIR := src
INC_DIR := include
TEST_DIR := tests
BUILD_DIR := build
BIN_DIR := bin

# Tools
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -I$(INC_DIR) -I.
LDFLAGS :=

# Coverage flags (used in coverage target)
COVERAGE_FLAGS := -fprofile-arcs -ftest-coverage

# Files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_BIN := $(BIN_DIR)/tests

# External single-header Catch2 expected at repo root as catch.hpp

.PHONY: all test lint static memcheck coverage doc clean debug

all: $(TEST_BIN)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJS) $(TEST_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) $(TEST_SRCS) -o $(TEST_BIN) $(LDFLAGS)

test: $(TEST_BIN)
	$(TEST_BIN)

lint:
	@echo "Running cpplint..."
	@python3 cpplint.py --filter=-build/include_subdir --exclude=catch.hpp $(SRC_DIR)/*.cpp $(INC_DIR)/*.hpp $(TEST_DIR)/*.cpp || true

static:
	@echo "Running cppcheck..."
	@mkdir -p $(BUILD_DIR)
	@cppcheck --enable=warning --inconclusive --std=c++17 -I $(INC_DIR) $(SRC_DIR) $(TEST_DIR) 2> $(BUILD_DIR)/cppcheck-report.txt || true
	@echo "Report: $(BUILD_DIR)/cppcheck-report.txt"

memcheck: CXXFLAGS += -g -O0
memcheck: $(TEST_BIN)
	@echo "Running valgrind memcheck..."
	valgrind --leak-check=full --show-leak-kinds=all $(TEST_BIN)

coverage: CXXFLAGS += -g -O0 $(COVERAGE_FLAGS)
coverage: LDFLAGS += -lgcov
coverage: clean $(TEST_BIN)
	@echo "Running tests with coverage..."
	$(TEST_BIN)
	@echo "Capturing lcov data..."
	lcov --capture --directory . --output-file $(BUILD_DIR)/coverage.info
	# Remove system and third-party files, keep project code
	lcov --remove $(BUILD_DIR)/coverage.info '/usr/*' '*/catch.hpp' --output-file $(BUILD_DIR)/coverage_project.info || true
	genhtml $(BUILD_DIR)/coverage_project.info --output-directory $(BUILD_DIR)/coverage_html --no-source --ignore-errors source --synthesize-missing
	@echo "Coverage report: $(BUILD_DIR)/coverage_html/index.html"

doc:
	@echo "Generating documentation with Doxygen..."
	doxygen Doxyfile

debug: CXXFLAGS += -g -O0
debug: $(TEST_BIN)
	gdb --args $(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) *.gcno *.gcda *.gcov


