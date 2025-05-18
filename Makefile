# build/test_load   → 測試 LoadModelCtx stub
# build/test_audio  → 測試 AudioInput 子模組
# build/test_mel    → 測試 PCM→Mel 子模組

CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -I./src               # 共用編譯旗標
LDLIBS  ?=

SRC_DIR := src
BLD_DIR := build

# ---------- 個別 target ----------
# test_load
$(BLD_DIR)/test_load: \
        $(SRC_DIR)/load_ctx.c \
        $(SRC_DIR)/stub_kernels.c \
        $(SRC_DIR)/test_load_ctx.c
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# test_audio
$(BLD_DIR)/test_audio: \
        $(SRC_DIR)/audio_in.c \
        $(SRC_DIR)/test_audio.c
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# test_mel
$(BLD_DIR)/test_mel: \
        $(SRC_DIR)/audio_in.c \
        $(SRC_DIR)/pcm_to_mel.c \
        $(SRC_DIR)/test_mel.c
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# pipeline_test
$(BLD_DIR)/pipeline_test: \
        $(SRC_DIR)/load_ctx.c \
        $(SRC_DIR)/stub_kernels.c \
        $(SRC_DIR)/audio_in.c \
        $(SRC_DIR)/pcm_to_mel.c \
        $(SRC_DIR)/pipeline_test.c
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $^ -o $@


# ---------- 群組 target ----------

all: $(BLD_DIR)/test_load $(BLD_DIR)/test_audio $(BLD_DIR)/test_mel $(BLD_DIR)/pipeline_test

# 方便只跑單一測試
test_load :  $(BLD_DIR)/test_load
test_audio:  $(BLD_DIR)/test_audio
test_mel  :  $(BLD_DIR)/test_mel
pipeline_test: $(BLD_DIR)/pipeline_test


clean:
	@rm -rf $(BLD_DIR)

.PHONY: all clean test_load test_audio test_mel 
# =====================================================================
