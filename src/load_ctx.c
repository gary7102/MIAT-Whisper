#include <stddef.h>           /* for NULL */
#include "stub_kernels.h"

typedef enum {
    LOAD_START,
    MAP_TENSOR,
    ALLOC_ARENA,
    THREAD_INIT,
    LOAD_DONE,
    LOAD_ERROR
} load_state_t;

int whisper_load_ctx(const char *model_path,
                     struct whisper_context **out_ctx) {
    load_state_t st = LOAD_START;
    struct whisper_context *ctx = NULL;
    int rc = 0;

    while (st != LOAD_DONE && st != LOAD_ERROR) {
        switch (st) {
        case LOAD_START: {
            rc = parse_header(model_path, &ctx);
            st = (rc == 0) ? MAP_TENSOR : LOAD_ERROR;
            break;
        }
        case MAP_TENSOR: {
            rc = map_weights(ctx);
            st = (rc == 0) ? ALLOC_ARENA : LOAD_ERROR;
            break;
        }
        case ALLOC_ARENA: {
            rc = alloc_arena(ctx);
            st = (rc == 0) ? THREAD_INIT : LOAD_ERROR;
            break;
        }
        case THREAD_INIT: {
            rc = init_thread_pool(ctx);
            st = (rc == 0) ? LOAD_DONE : LOAD_ERROR;
            break;
        }
        default:
            st = LOAD_ERROR;
        }
    }

    if (st == LOAD_DONE) {
        *out_ctx = ctx;
        return 0;
    }
    free_ctx(ctx);
    return -1;
}

