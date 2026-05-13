CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Isakura
LDFLAGS = -lm

# Source files for forest
FOREST_SRCS = src/main.c src/forest.c src/noise.c src/render.c
# Source files for sakura demo
SAKURA_SRCS = src/saku.c sakura/sakura.c src/render.c

# Output directories
BUILD_DIR = build
DIST_DIR = dist

# Detect OS
ifeq ($(OS),Windows_NT)
    EXT = .exe
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = rmdir /s /q
else
    EXT =
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf
endif

FOREST_BIN = $(BUILD_DIR)/forest$(EXT)
SAKURA_BIN = $(BUILD_DIR)/sakura$(EXT)

all: $(BUILD_DIR) $(FOREST_BIN) $(SAKURA_BIN)

$(BUILD_DIR):
	$(MKDIR)

$(FOREST_BIN): $(FOREST_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(FOREST_SRCS) -o $@ $(LDFLAGS)

$(SAKURA_BIN): $(SAKURA_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SAKURA_SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) $(BUILD_DIR)

.PHONY: all clean
