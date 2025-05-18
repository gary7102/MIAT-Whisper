#pragma once
#include <stddef.h>
#include <stdint.h>

struct pcm_buffer {
    int16_t *data;        /* 16-bit samples              */
    size_t   n_samples;   /* 總取樣點                    */
    int      sample_rate; /* 期望 = 16000                */
};

/* 傳回 0=成功; 非 0=失敗 */
int audio_input_load(const char *wav_path, struct pcm_buffer *out_pcm);

/* 釋放內部 malloc 的資料 */
void audio_input_free(struct pcm_buffer *pcm);
