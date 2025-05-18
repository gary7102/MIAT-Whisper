/* src/test_main.c */
#include <stdio.h>
#include "stub_kernels.h"

int whisper_load_ctx(const char *model_path,
                     struct whisper_context **out_ctx);  // 來自 load_ctx.c

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <model.bin>\n", argv[0]);
        return 1;
    }
    struct whisper_context *ctx = NULL;
    if (whisper_load_ctx(argv[1], &ctx) == 0) {
        printf("[test] Load ctx OK\n");
        free_ctx(ctx);
        return 0;               // 成功
    }
    return 1;                   // 失敗
}
