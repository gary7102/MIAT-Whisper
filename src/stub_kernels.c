/* src/stub_kernels.c */
#include "stub_kernels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ggml/ggml.h"

/* ----- 極簡結構 (之後會擴充) ----- */
struct whisper_context {
    void *arena;
};

int parse_header(const char *path, struct whisper_context **ctx_out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "[stub] cannot open model: %s\n", path);
        return -1;
    }
    fclose(fp);
    *ctx_out = calloc(1, sizeof(struct whisper_context));
    printf("[stub] header OK (stub)\n");
    return 0;
}

int map_weights(struct whisper_context *ctx) {
    (void)ctx;
    printf("[stub] weights mapped (stub)\n");
    return 0;
}

int alloc_arena(struct whisper_context *ctx) {
    ctx->arena = malloc(1);           /* 先佔 1 byte */
    printf("[stub] arena alloc OK (stub)\n");
    return ctx->arena ? 0 : -1;
}

int init_thread_pool(struct whisper_context *ctx) {
    (void)ctx;                        /* 單執行緒 */
    printf("[stub] thread init OK (single-thread)\n");
    return 0;
}

void free_ctx(struct whisper_context *ctx) {
    if (!ctx) return;
    free(ctx->arena);
    free(ctx);
}

