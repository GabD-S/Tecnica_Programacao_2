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
APP_BIN := $(BIN_DIR)/tp2_cli

# Tools
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -I$(INC_DIR) -I.
LDFLAGS :=

# Coverage flags (used in coverage target)
COVERAGE_FLAGS := -fprofile-arcs -ftest-coverage

# Files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
# Separate application main from core sources to avoid linking main into tests
APP_MAIN := $(SRC_DIR)/main.cpp
CORE_SRCS := $(filter-out $(APP_MAIN),$(SRCS))
CORE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CORE_SRCS))
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_BIN := $(BIN_DIR)/tests

# External single-header Catch2 expected at repo root as catch.hpp

.PHONY: all test lint static memcheck coverage doc clean debug run

all: $(TEST_BIN)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

	# Build unit test runner

# Build unit test runner (link only core objects; Catch2 provides main)
$(TEST_BIN): $(CORE_OBJS) $(TEST_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_OBJS) $(TEST_SRCS) -o $(TEST_BIN) $(LDFLAGS)

# Build CLI app if main.cpp exists
ifeq (,$(wildcard $(SRC_DIR)/main.cpp))
$(APP_BIN): | $(BIN_DIR)
	@echo "No src/main.cpp, CLI not built (expected for initial RED)."
else
$(APP_BIN): $(CORE_OBJS) $(SRC_DIR)/main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_OBJS) $(SRC_DIR)/main.cpp -o $(APP_BIN) $(LDFLAGS)
endif

test: $(TEST_BIN) $(APP_BIN)
	$(TEST_BIN)

run: $(APP_BIN)
	$(APP_BIN)

lint:
	@echo "Running cpplint..."
	# Quote paths to handle spaces and use find to expand files robustly
	@python3 cpplint.py --filter=-build/include_subdir --exclude=catch.hpp \
		`find "$(SRC_DIR)" -maxdepth 1 -name '*.cpp' -print` \
		`find "$(INC_DIR)" -maxdepth 1 -name '*.hpp' -print` \
		`find "$(TEST_DIR)" -maxdepth 1 -name '*.cpp' -print` || true

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
	lcov --remove $(BUILD_DIR)/coverage.info '/usr/*' '*/catch.hpp' '*/tests/*' '*/build/*' --output-file $(BUILD_DIR)/coverage_project.info || true
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


