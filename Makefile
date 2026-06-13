CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Isrc -Isrc/objects
LDLIBS  := -lm

# raylib: prefer pkg-config (it pulls in the right static deps), otherwise
# fall back to linking the shared library directly (e.g. apt libraylib-dev).
ifeq ($(shell pkg-config --exists raylib 2>/dev/null && echo yes),yes)
  CFLAGS += $(shell pkg-config --cflags raylib)
  LDLIBS += $(shell pkg-config --libs raylib)
else
  LDLIBS += -lraylib
endif

SRC := src/main.c src/physics.c src/render.c src/scenarios.c \
       src/objects/body.c src/objects/obj_types.c
BUILD_DIR := build
OBJ := $(SRC:%.c=$(BUILD_DIR)/%.o)

TARGET := nbody

ifeq ($(OS),Windows_NT)
  MKDIR_P = if not exist "$(subst /,\,$(patsubst %/,%,$(dir $@)))" mkdir "$(subst /,\,$(patsubst %/,%,$(dir $@)))"
  CLEAN_BUILD = if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
  CLEAN_TARGET = if exist "$(TARGET)" del /q "$(TARGET)"
  CLEAN_TARGET_EXE = if exist "$(TARGET).exe" del /q "$(TARGET).exe"
else
  MKDIR_P = mkdir -p $(dir $@)
  CLEAN_BUILD = rm -rf $(BUILD_DIR)
  CLEAN_TARGET = rm -f $(TARGET)
  CLEAN_TARGET_EXE =
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(CLEAN_BUILD)
	$(CLEAN_TARGET)
	$(CLEAN_TARGET_EXE)

# --- web / WASM build --------------------------------------------------------
# Requires Emscripten in PATH and a raylib PLATFORM_WEB static build.
# Usage:  make web RAYLIB_WEB=/path/to/raylib-web
#
# The CI workflow builds raylib for web and passes its output directory here.
RAYLIB_WEB ?= raylib-web
WEB_DIR    := web

WEB_FLAGS := -Wall -Wextra -Os -std=c11 -Isrc -Isrc/objects \
             -DPLATFORM_WEB \
             -I$(RAYLIB_WEB)/include \
             -s USE_GLFW=3 \
             -s TOTAL_MEMORY=67108864 \
             -s ALLOW_MEMORY_GROWTH=1 \
             --preload-file assets \
             --shell-file $(WEB_DIR)/shell.html

web: $(WEB_DIR)/index.html

$(WEB_DIR)/index.html: $(SRC) $(WEB_DIR)/shell.html
	emcc $(SRC) $(RAYLIB_WEB)/lib/libraylib.a $(WEB_FLAGS) -o $@

.PHONY: all clean web
