# CC      := gcc
# CFLAGS  := -O2 -std=c11 -I./src
# SRC     := src/load_ctx.c src/stub_kernels.c src/test_main.c
# OUT     := build/test_load

# $(OUT): $(SRC)
# 	@mkdir -p build
# 	$(CC) $(CFLAGS) $^ -o $@

# clean:
# 	rm -rf build
############################################################
CC      := gcc
CFLAGS  := -O2 -std=c11 -I./src
# 新增檔案 ▼
SRC     := src/audio_in.c src/test_audio.c
OUT     := build/test_audio

$(OUT): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
