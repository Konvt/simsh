INTERPRETER_SRC_DIR ?= interpreter-impl/src
INTERPRETER_UTIL_DIR ?= interpreter-impl/util
SHELL_SRC_DIR ?= shell-impl/src
BUILD_DIR ?= build
TARGET ?= simsh

CC := g++
OPT_LEVEL ?= O3
CC_STANDARD ?= c++20

CFLAGS := -std=$(CC_STANDARD) -Wall -Wpedantic -Wextra -Wshadow -$(OPT_LEVEL) -I$(INTERPRETER_SRC_DIR) -I$(INTERPRETER_UTIL_DIR) -I$(SHELL_SRC_DIR)

INTERPRETER_SRC := $(wildcard $(INTERPRETER_SRC_DIR)/*.cpp)
INTERPRETER_UTIL := $(wildcard $(INTERPRETER_UTIL_DIR)/*.cpp)
SHELL_SRC := $(wildcard $(SHELL_SRC_DIR)/*.cpp)

INTERPRETER_SRC_OBJ := $(patsubst $(INTERPRETER_SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(INTERPRETER_SRC))
INTERPRETER_UTIL_OBJ := $(patsubst $(INTERPRETER_UTIL_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(INTERPRETER_UTIL))
SHELL_SRC_OBJ := $(patsubst $(SHELL_SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SHELL_SRC))
DEP := $(INTERPRETER_SRC_OBJ:.o=.d) $(INTERPRETER_UTIL_OBJ:.o=.d) $(SHELL_SRC_OBJ:.o=.d)

all: $(TARGET)
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
rebuild: clean all
debug:
	$(MAKE) -s rebuild OPT_LEVEL=g && gdb -q -tui $(TARGET)
-include $(DEP)

$(TARGET): $(INTERPRETER_SRC_OBJ) $(INTERPRETER_UTIL_OBJ) $(SHELL_SRC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(INTERPRETER_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)
$(BUILD_DIR)/%.o: $(INTERPRETER_UTIL_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)
$(BUILD_DIR)/%.o: $(SHELL_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)
