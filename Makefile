CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lm

# Source files
SRCS = src/main.c src/forest.c src/noise.c src/render.c
# Sakura is currently not used in main.c, but we can include it if needed.
# For now, let's keep it to what main.c uses.

# Output binary name
TARGET = forest

# Detect OS
ifeq ($(OS),Windows_NT)
    BIN = $(TARGET).exe
    # Windows might need extra flags, but usually standard gcc works for this project.
else
    BIN = $(TARGET)
endif

# Output directory
DIST_DIR = dist

all: $(DIST_DIR)/$(BIN)

$(DIST_DIR)/$(BIN): $(SRCS)
	@mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(DIST_DIR)

.PHONY: all clean
