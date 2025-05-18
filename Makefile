CC      := gcc
CFLAGS  := -O2 -std=c11 -I./src
SRC     := src/load_ctx.c src/stub_kernels.c src/test_main.c
OUT     := build/test_load

$(OUT): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
