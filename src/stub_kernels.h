/* src/stub_kernels.h */
#pragma once
#include <stdint.h>

struct whisper_context;               /* 前向宣告 */

int  parse_header    (const char *path, struct whisper_context **ctx_out);
int  map_weights     (struct whisper_context *ctx);
int  alloc_arena     (struct whisper_context *ctx);
int  init_thread_pool(struct whisper_context *ctx);
void free_ctx        (struct whisper_context *ctx);

