#include "audio_in.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* --- 子 Grafcet 對應的狀態列舉 --- */
typedef enum {
    AI_START,
    AI_OPEN,
    AI_RESAMPLE,  /* stub: 直接 pass-through */
    AI_MONOMIX,   /* stub: 若 stereo -> 取 L */
    AI_NORMALIZE, /* stub: no-op */
    AI_DONE,
    AI_ERROR
} ai_state_t;

/* --- 簡易 WAV header 結構 (PCM 16-bit) --- */
#pragma pack(push, 1)
struct wav_header {
    char     riff[4];      /* "RIFF" */
    uint32_t size;
    char     wave[4];      /* "WAVE" */
    char     fmt_[4];      /* "fmt " */
    uint32_t fmt_size;     /* 16 for PCM */
    uint16_t audio_fmt;    /* 1 = PCM */
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_tag[4];  /* "data" */
    uint32_t data_size;
};
#pragma pack(pop)

/* ---------- 實作 ---------- */

int audio_input_load(const char *wav_path, struct pcm_buffer *out_pcm) {
    ai_state_t st = AI_START;
    FILE *fp = NULL;
    struct wav_header hdr;
    size_t read_samples = 0;
    int16_t *pcm_data = NULL;

    while (st != AI_DONE && st != AI_ERROR) {
        switch (st) {
        case AI_START:
            st = AI_OPEN;
            break;

        case AI_OPEN: {
            fp = fopen(wav_path, "rb");
            if (!fp) { perror("[audio] fopen"); st = AI_ERROR; break; }

            /* 1) 讀 RIFF & WAVE 標籤 */
            char tag[4];
            uint32_t chunk_size;
            if (fread(tag, 1, 4, fp) != 4 || memcmp(tag, "RIFF", 4)) {
                fprintf(stderr, "[audio] not RIFF\n"); st = AI_ERROR; break;
            }
            fseek(fp, 4, SEEK_CUR);      /* skip file size */
            if (fread(tag, 1, 4, fp) != 4 || memcmp(tag, "WAVE", 4)) {
                fprintf(stderr, "[audio] not WAVE\n"); st = AI_ERROR; break;
            }

            /* 2) 迴圈尋找 "fmt " 及 "data" chunk */
            uint16_t num_channels = 0, bits_per_sample = 0;
            uint32_t sample_rate  = 0, data_size = 0;
            while (fread(tag, 1, 4, fp) == 4 &&
                fread(&chunk_size, 4, 1, fp) == 1) {
                if (memcmp(tag, "fmt ", 4) == 0) {
                    /* 讀 PCM 格式欄位 (我們只關心前 16 bytes) */
                    uint16_t audio_fmt;
                    fread(&audio_fmt,      2, 1, fp);
                    fread(&num_channels,   2, 1, fp);
                    fread(&sample_rate,    4, 1, fp);
                    fseek(fp, 6, SEEK_CUR);           /* skip byteRate & align */
                    fread(&bits_per_sample, 2, 1, fp);
                    fseek(fp, chunk_size - 16, SEEK_CUR);  /* skip rest */
                } else if (memcmp(tag, "data", 4) == 0) {
                    data_size = chunk_size;
                    break;                              /* 找到資料 */
                } else {
                    /* 跳過不相關 chunk */
                    fseek(fp, chunk_size, SEEK_CUR);
                }
            }

            /* 3) 基本驗證 */
            if (data_size == 0 || bits_per_sample != 16 ||
                (num_channels != 1 && num_channels != 2)) {
                fprintf(stderr, "[audio] unsupported wav\n"); st = AI_ERROR; break;
            }
            /* 把收集到的 meta 存到臨時變數，後續 state 使用 */
            hdr.sample_rate  = sample_rate;
            hdr.num_channels = num_channels;
            hdr.data_size    = data_size;
            st = AI_RESAMPLE;
            break;
        }

        case AI_RESAMPLE:       /* MVP: 僅接受 16 kHz */
            if (hdr.sample_rate != 16000) {
                fprintf(stderr, "[audio] need 16kHz (got %u)\n", hdr.sample_rate);
                st = AI_ERROR;
                break;
            }
            st = AI_MONOMIX;
            break;

        case AI_MONOMIX:
            /* 若為立體聲取左聲道；MVP 僅支援 1 or 2 ch */
            if (hdr.num_channels != 1 && hdr.num_channels != 2) {
                fprintf(stderr, "[audio] need mono/stereo\n");
                st = AI_ERROR;
                break;
            }
            pcm_data = malloc(hdr.data_size / hdr.num_channels);
            if (!pcm_data) {
                perror("[audio] malloc");
                st = AI_ERROR;
                break;
            }
            if (hdr.num_channels == 1) {
                /* 直接讀取 */
                read_samples = fread(pcm_data, 1, hdr.data_size, fp) / 2;
            } else { /* stereo → 取 Left */
                size_t frames = hdr.data_size / 4; /* 2ch * 16-bit */
                for (size_t i = 0; i < frames; ++i) {
                    int16_t sample[2];
                    fread(sample, 2, 2, fp);
                    pcm_data[i] = sample[0];
                }
                read_samples = frames;
            }
            st = AI_NORMALIZE;
            break;

        case AI_NORMALIZE:
            /* MVP: no-op，僅填 meta */
            out_pcm->data        = pcm_data;
            out_pcm->n_samples   = read_samples;
            out_pcm->sample_rate = 16000;
            st = AI_DONE;
            break;

        default:
            st = AI_ERROR;
        }
    }

    if (st == AI_DONE) {
        if (fp) fclose(fp);
        return 0;
    }
    if (fp) fclose(fp);
    free(pcm_data);
    return -1;
}

void audio_input_free(struct pcm_buffer *pcm) {
    if (!pcm) return;
    free(pcm->data);
    memset(pcm, 0, sizeof(*pcm));
}
