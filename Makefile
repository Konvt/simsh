# This file is licensed under the MIT License.
# Please see the LICENSE file in the root of the repository for the full license text.
# Copyright (c) 2024-2025 Konvt

SRC_DIR := src
INC_DIR := inc
BUILD_BASE := build
TARGET := tish

CC := g++
CC_STANDARD := c++20

BUILD_TYPE ?= debug
BUILD_TYPE_LOWER := $(shell echo $(BUILD_TYPE) | tr A-Z a-z)
ifeq ($(BUILD_TYPE_LOWER),release)
  OPT_LEVEL := O3
  CFLAGS := -std=$(CC_STANDARD) -Wall -Wpedantic -Wextra -Wshadow -DNDEBUG -$(OPT_LEVEL) -I$(INC_DIR)
else ifeq ($(BUILD_TYPE_LOWER),debug)
  OPT_LEVEL := g
  CFLAGS := -std=$(CC_STANDARD) -Wall -Wpedantic -Wextra -Wshadow -$(OPT_LEVEL) -I$(INC_DIR)
else
  $(error Unsupported BUILD_TYPE '$(BUILD_TYPE)', please use 'debug' or 'release' (case insensitive))
endif
BUILD_DIR := $(BUILD_BASE)/$(BUILD_TYPE)

SRC := $(shell find $(SRC_DIR) -name '*.cpp')
SRC_OBJ := $(subst $(SRC_DIR)/, $(BUILD_DIR)/, $(SRC:.cpp=.o))
DEP := $(SRC_OBJ:.o=.d)

all: debug
debug: $(TARGET)
release:
	$(MAKE) -j BUILD_TYPE=release $(TARGET)
clean:
	rm -rf $(BUILD_BASE) $(TARGET)
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
