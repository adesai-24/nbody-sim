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

SRC := src/main.c src/physics.c src/render.c \
       src/objects/body.c src/objects/obj_types.c
OBJ := $(SRC:.c=.o)

TARGET := orrery

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
