#include "pcm_to_mel.h"
#include "audio_in.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* --- FSM 狀態 --- */
typedef enum {
    PM_START,
    PM_FRAME_WIN,
    PM_FFT,         /* MVP: dummy */
    PM_MEL_BANK,    /* MVP: dummy */
    PM_LOG_NORM,    /* MVP: set zero */
    PM_DONE,
    PM_ERROR
} pm_state_t;

/* 25 ms 窗；10 ms stride → 400/160 跟 16 kHz 對應 */
#define WIN_SIZE   400
#define WIN_STRIDE 160
#define MEL_DIM    80

/* MVP: 全零 Mel。計算 n_frames 後 malloc 大小即可 */
int pcm_to_mel_extract(const struct pcm_buffer *pcm,
                       struct mel_matrix      *out_mel)
{
    if (!pcm || !pcm->data || pcm->sample_rate != 16000) {
        fprintf(stderr, "[mel] need 16 kHz PCM\n");
        return -1;
    }
    /* 計算幀數 */
    size_t total = pcm->n_samples;
    size_t n_frames = (total < WIN_SIZE) ? 0
                        : 1 + (total - WIN_SIZE) / WIN_STRIDE;

    if (n_frames == 0) {
        fprintf(stderr, "[mel] audio too short\n");
        return -1;
    }
    float *mel = calloc(n_frames * MEL_DIM, sizeof(float));
    if (!mel) return -1;

    /* MVP FSM 其實只做狀態流轉，不變更資料 */
    pm_state_t st = PM_START;
    size_t f = 0;
    while (st != PM_DONE && st != PM_ERROR) {
        switch (st) {
        case PM_START:
            st = PM_FRAME_WIN;
            break;

        case PM_FRAME_WIN:
            /* normally: apply Hanning window here */
            st = PM_FFT;
            break;

        case PM_FFT:
            /* normally: FFT -> power spectrum */
            st = PM_MEL_BANK;
            break;

        case PM_MEL_BANK:
            /* normally: multiply Mel filterbank */
            st = PM_LOG_NORM;
            break;

        case PM_LOG_NORM:
            /* MVP: leave zeros */
            if (++f >= n_frames) st = PM_DONE;
            else st = PM_FRAME_WIN;
            break;

        default:
            st = PM_ERROR;
        }
    }

    if (st == PM_DONE) {
        out_mel->data     = mel;
        out_mel->n_frames = n_frames;
        return 0;
    }
    free(mel);
    return -1;
}

void mel_free(struct mel_matrix *mel) {
    if (!mel) return;
    free(mel->data);
    memset(mel, 0, sizeof(*mel));
}
