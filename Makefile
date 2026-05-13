CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Isakura
LDFLAGS = -lm

# Source files for forest
FOREST_SRCS = src/main.c src/forest.c src/noise.c src/render.c
# Source files for sakura demo
SAKURA_SRCS = src/sakura_demo.c sakura/sakura.c src/render.c

# Output directory
DIST_DIR = dist

# Detect OS
ifeq ($(OS),Windows_NT)
    EXT = .exe
else
    EXT =
endif

FOREST_BIN = $(DIST_DIR)/forest$(EXT)
SAKURA_BIN = $(DIST_DIR)/sakura$(EXT)

all: $(FOREST_BIN) $(SAKURA_BIN)

$(FOREST_BIN): $(FOREST_SRCS)
	@mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) $(FOREST_SRCS) -o $@ $(LDFLAGS)

$(SAKURA_BIN): $(SAKURA_SRCS)
	@mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) $(SAKURA_SRCS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(DIST_DIR)

.PHONY: all clean
