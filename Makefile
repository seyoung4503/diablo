# Diablo 1 Clone - Makefile
CC = clang
CFLAGS = -std=c11 -Wall -Wextra -g $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN = $(BUILD_DIR)/diablo

# Find all .c files recursively in src/
SRCS = $(shell find $(SRC_DIR) -name '*.c')
# Convert src/foo/bar.c -> build/foo/bar.o
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean run debug

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Pattern rule: build/*.o from src/*.c, preserving subdirectory structure
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -MP -c $< -o $@

# Include auto-generated dependency files
-include $(DEPS)

run: $(BIN)
	./$(BIN)

debug: $(BIN)
	lldb ./$(BIN)

clean:
	rm -rf $(BUILD_DIR)
