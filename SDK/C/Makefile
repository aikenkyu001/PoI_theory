CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O3
LDFLAGS = -shared
TARGET = libnoetics.so
WASM_TARGET = noetics.js
SRCS = noetics.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

wasm: $(SRCS)
	emcc $(SRCS) -o $(WASM_TARGET) \
		-s EXPORTED_FUNCTIONS='["_noetics_create_context","_noetics_key_create","_noetics_step_unified","_noetics_compute_energy","_wasm_noetics_compute_entropy","_wasm_noetics_detect_rank_jump","_wasm_noetics_export_structure","_wasm_noetics_import_structure","_wasm_noetics_key_data_ptr","_wasm_noetics_assign_sector_indices","_wasm_noetics_compute_sector_energy","_malloc","_free"]' \
		-s EXPORTED_RUNTIME_METHODS='["cwrap","ccall","HEAPF64","HEAP32"]' \
		-s MODULARIZE=1 -s EXPORT_NAME='NoeticsModule' \
		-O3

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
