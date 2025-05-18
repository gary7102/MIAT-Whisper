# Makefile
CC      := gcc
CFLAGS  := -O2 -std=c11 -I./src -I./src/ggml/include
SRC     := src/load_ctx.c src/stub_kernels.c src/ggml/src/ggml.c
OUT     := build/test_load

$(OUT): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -lm -pthread -o $@

clean:
	rm -rf build

