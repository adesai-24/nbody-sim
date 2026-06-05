# Standalone profiling harness. Links only the raylib-free core
# (physics.c, body.c, obj_types.c) so it builds and runs headless.
CC     := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11 -I../src -I../src/objects
LDLIBS := -lm

CORE := ../src/physics.c ../src/objects/body.c ../src/objects/obj_types.c

profile_sim: profile_sim.c $(CORE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f profile_sim

.PHONY: clean
