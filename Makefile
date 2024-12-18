# This file is licensed under the MIT License.
# Please see the LICENSE file in the root of the repository for the full license text.
# Copyright (c) 2024 Konvt

SRC_DIR := src
UTIL_DIR := util
BUILD_DIR := build
TARGET ?= simsh

CC := g++
OPT_LEVEL ?= O3
CC_STANDARD := c++20

CFLAGS := -std=$(CC_STANDARD) -Wall -Wpedantic -Wextra -Wshadow -$(OPT_LEVEL) -I$(SRC_DIR) -I$(UTIL_DIR)

SRC := $(wildcard $(SRC_DIR)/*.cpp)
UTIL := $(wildcard $(UTIL_DIR)/*.cpp)

SRC_OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC))
UTIL_OBJ := $(patsubst $(UTIL_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(UTIL))
DEP := $(SRC_OBJ:.o=.d) $(UTIL_OBJ:.o=.d)

all: $(TARGET)
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
rebuild: clean all
debug:
	$(MAKE) -s rebuild OPT_LEVEL=g && gdb -q -tui $(TARGET)
format:
	clang-format -i $(SRC_DIR)/*.*pp $(UTIL_DIR)/*.*pp
-include $(DEP)

$(TARGET): $(SRC_OBJ) $(UTIL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)
$(BUILD_DIR)/%.o: $(UTIL_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)
