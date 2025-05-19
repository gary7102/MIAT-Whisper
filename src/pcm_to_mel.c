
// /* src/pcm_to_mel.c  – Whisper 80-bin log-Mel extractor (16 kHz) */
// #include "pcm_to_mel.h"
// #include "audio_in.h"
// #include "kissfft/kiss_fft.h"
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <math.h>

// #define WIN_SIZE    400       /* 25 ms */
// #define WIN_STRIDE  160       /* 10 ms */
// #define N_FFT       512
// #define N_FREQ      (N_FFT/2 + 1)   /* 257 */
// #define MEL_DIM     80
// #define PI_F        3.14159265358979f

// /* ───────── Hanning window (預運算) ───────── */
// static float hann[WIN_SIZE];
// static void init_hann(void) {
//     for (int i = 0; i < WIN_SIZE; ++i)
//         hann[i] = 0.5f * (1.0f - cosf(2.0f*PI_F*i/(WIN_SIZE-1)));
// }

// /* ───────── Mel filter bank ───────── */
// /* 參考 whisper.cpp build_mel_filter_bank_f32() */
// static float mel_fb[MEL_DIM][N_FREQ];
// /* build_mel_fb(): 依論文 & whisper.cpp 移植 */
// static void build_mel_fb(void) {
//     const float f_min =   0.0f;
//     const float f_max = 8000.0f;
//     /* Convert hz -> mel */
//     float mel_min = 2595.0f * log10f(1.0f + f_min/700.0f);
//     float mel_max = 2595.0f * log10f(1.0f + f_max/700.0f);

//     /* mel spacing */
//     float mel_pts[MEL_DIM+2];
//     for (int m = 0; m < MEL_DIM+2; ++m) {
//         mel_pts[m] = mel_min + (mel_max - mel_min) * m / (MEL_DIM+1);
//     }

//     /* mel -> hz */
//     float hz_pts[MEL_DIM+2];
//     for (int m = 0; m < MEL_DIM+2; ++m) {
//         hz_pts[m] = 700.0f * (powf(10.0f, mel_pts[m]/2595.0f) - 1.0f);
//     }

//     /* hz -> fft bin */
//     int bin[MEL_DIM+2];
//     for (int m = 0; m < MEL_DIM+2; ++m) {
//         bin[m] = (int) floorf((N_FREQ*2-2) * hz_pts[m] / (f_max*2));
//     }

//     /* build triangular filters */
//     memset(mel_fb, 0, sizeof(mel_fb));
//     for (int m = 1; m <= MEL_DIM; ++m) {
//         int start = bin[m-1], center = bin[m], end = bin[m+1];
//         for (int k = start; k < center; ++k) {
//             mel_fb[m-1][k] = (float)(k - start) / (center - start);
//         }
//         for (int k = center; k < end; ++k) {
//             mel_fb[m-1][k] = (float)(end - k)   / (end - center);
//         }
//     }

//     /* —— 新增：對每個 filter 做歸一化 —— */
//     for (int m = 0; m < MEL_DIM; ++m) {
//         float sum = 0.0f;
//         for (int k = 0; k < N_FREQ; ++k) {
//             sum += mel_fb[m][k];
//         }
//         if (sum > 0.0f) {
//             for (int k = 0; k < N_FREQ; ++k) {
//                 mel_fb[m][k] /= sum;
//             }
//         }
//     }
// }

// // static void build_mel_fb(void) {
// //     const float f_min =   0.0f;
// //     const float f_max = 8000.0f;        /* Nyquist 16 kHz */
// //     float mel_min = 2595.0f * log10f(1.0f + f_min/700.0f);
// //     float mel_max = 2595.0f * log10f(1.0f + f_max/700.0f);

// //     float mel_points[MEL_DIM+2];
// //     for (int m = 0; m < MEL_DIM+2; ++m)
// //         mel_points[m] = mel_min + (mel_max-mel_min)*m/(MEL_DIM+1);

// //     float hz_points[MEL_DIM+2];
// //     for (int m = 0; m < MEL_DIM+2; ++m)
// //         hz_points[m] = 700.0f * (powf(10.0f, mel_points[m]/2595.0f) - 1.0f);

// //     int bin[MEL_DIM+2];
// //     for (int m = 0; m < MEL_DIM+2; ++m)
// //         bin[m] = (int) floorf((N_FFT+1) * hz_points[m] / 16000.0f);

// //     memset(mel_fb, 0, sizeof(mel_fb));
// //     for (int m = 1; m <= MEL_DIM; ++m) {
// //         for (int k = bin[m-1]; k < bin[m]; ++k)
// //             mel_fb[m-1][k] = (float)(k - bin[m-1]) / (bin[m]-bin[m-1]);
// //         for (int k = bin[m]; k < bin[m+1]; ++k)
// //             mel_fb[m-1][k] = (float)(bin[m+1]-k) / (bin[m+1]-bin[m]);
// //     }

// //     // for (int m = 0; m < MEL_DIM; ++m) {
// //     //     float sum = 0.f;
// //     //     for (int k = 0; k < N_FREQ; ++k) sum += mel_fb[m][k];
// //     //     if (sum > 0) {
// //     //         for (int k = 0; k < N_FREQ; ++k) mel_fb[m][k] /= sum;
// //     //     }
// //     // }


// // }

// /* ───────── Main extractor ───────── */
// int pcm_to_mel_extract(const struct pcm_buffer *pcm,
//                        struct mel_matrix      *out_mel)
// {
//     if (!pcm || !pcm->data || pcm->sample_rate != 16000) {
//         fprintf(stderr, "[mel] need 16 kHz PCM\n");
//         return -1;
//     }
//     /* 計算 frame 數 */
//     size_t total = pcm->n_samples;
//     size_t n_frames = (total < WIN_SIZE) ? 0
//                           : 1 + (total - WIN_SIZE)/WIN_STRIDE;
//     if (n_frames == 0) { fprintf(stderr, "[mel] audio too short\n"); return -1; }

//     /* malloc output */
//     float *mel_out = malloc(n_frames * MEL_DIM * sizeof(float));
//     if (!mel_out) return -1;

//     /* 一次性初始化常量 */
//     static int  inited = 0;
//     static kiss_fft_cfg cfg = NULL;
//     static kiss_fft_cpx fft_in[N_FFT], fft_out[N_FFT];
//     static float power[N_FREQ];
//     if (!inited) {
//         init_hann();
//         build_mel_fb();
//         cfg = kiss_fft_alloc(N_FFT, 0, NULL, NULL);
//         inited = 1;
//     }

//     /* 逐 frame 處理 */
//     size_t offset = 0;
//     for (size_t f = 0; f < n_frames; ++f, offset += WIN_STRIDE) {
//         /* (1) window & zero-pad */
//         for (int i = 0; i < WIN_SIZE; ++i) {
//             fft_in[i].r = (float)pcm->data[offset+i] * (1.0f/32768.f) * hann[i];
//             fft_in[i].i = 0.0f;
//         }
//         for (int i = WIN_SIZE; i < N_FFT; ++i)
//             fft_in[i].r = fft_in[i].i = 0.0f;

//         /* (2) FFT */
//         kiss_fft(cfg, fft_in, fft_out);

//         // /* (3) power spectrum */
//         // for (int k = 0; k < N_FREQ; ++k)
//         //     power[k] = fft_out[k].r*fft_out[k].r + fft_out[k].i*fft_out[k].i;
//         /* (3) power spectrum –– 加上縮放 */
//         const float scale = 1.0f / (N_FFT * N_FFT);  /* 1 / 512² */
//         for (int k = 0; k < N_FREQ; ++k) {
//             power[k] = (fft_out[k].r*fft_out[k].r +
//                         fft_out[k].i*fft_out[k].i) * scale;
//         }

//         /* (4) Mel bins */
//         float *dst = mel_out + f*MEL_DIM;
//         for (int m = 0; m < MEL_DIM; ++m) {
//             float s = 0.0f;
//             for (int k = 0; k < N_FREQ; ++k)
//                 s += power[k] * mel_fb[m][k];
//             /* log10 + normalize (mean≈-4, std≈ 4) */
//             float v = log10f(fmaxf(s, 1e-10f));
//             dst[m] = (v + 4.0f) / 4.0f;
//         }
//     }

//     out_mel->data     = mel_out;
//     out_mel->n_frames = n_frames;
//     return 0;
// }

// void mel_free(struct mel_matrix *mel) {
//     if (!mel) return;
//     free(mel->data);
//     memset(mel, 0, sizeof(*mel));
// }
// #include "pcm_to_mel.h"
// #include "audio_in.h"
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include "kissfft/kiss_fft.h"

// #define MEL_DIM 80
// #define N_FFT   512
// #define N_FREQ  (N_FFT/2 + 1)   /* 257 */
// static float mel_fb[MEL_DIM][N_FREQ];
// static int mel_fb_init = 0;
// static void mel_build(int sample_rate) { ... }   /* 依 Whisper 算式 */


// /* --- FSM 狀態 --- */
// typedef enum {
//     PM_START,
//     PM_FRAME_WIN,
//     PM_FFT,         /* MVP: dummy */
//     PM_MEL_BANK,    /* MVP: dummy */
//     PM_LOG_NORM,    /* MVP: set zero */
//     PM_DONE,
//     PM_ERROR
// } pm_state_t;

// /* 25 ms 窗；10 ms stride → 400/160 跟 16 kHz 對應 */
// #define WIN_SIZE   400
// #define WIN_STRIDE 160
// #define MEL_DIM    80

// /* MVP: 全零 Mel。計算 n_frames 後 malloc 大小即可 */
// int pcm_to_mel_extract(const struct pcm_buffer *pcm,
//                        struct mel_matrix      *out_mel)
// {
//     if (!pcm || !pcm->data || pcm->sample_rate != 16000) {
//         fprintf(stderr, "[mel] need 16 kHz PCM\n");
//         return -1;
//     }
//     /* 計算幀數 */
//     size_t total = pcm->n_samples;
//     size_t n_frames = (total < WIN_SIZE) ? 0
//                         : 1 + (total - WIN_SIZE) / WIN_STRIDE;

//     if (n_frames == 0) {
//         fprintf(stderr, "[mel] audio too short\n");
//         return -1;
//     }
//     float *mel = calloc(n_frames * MEL_DIM, sizeof(float));
//     if (!mel) return -1;

//     /* MVP FSM 其實只做狀態流轉，不變更資料 */
//     pm_state_t st = PM_START;
//     size_t f = 0;
//     while (st != PM_DONE && st != PM_ERROR) {
//         switch (st) {
//         case PM_START:
//             st = PM_FRAME_WIN;
//             break;

//         case PM_FRAME_WIN:
//             /* normally: apply Hanning window here */
//             st = PM_FFT;
//             break;

//         case PM_FFT:
//             /* normally: FFT -> power spectrum */
//             st = PM_MEL_BANK;
//             break;

//         case PM_MEL_BANK:
//             /* normally: multiply Mel filterbank */
//             st = PM_LOG_NORM;
//             break;

//         case PM_LOG_NORM:
//             /* MVP: leave zeros */
//             if (++f >= n_frames) st = PM_DONE;
//             else st = PM_FRAME_WIN;
//             break;

//         default:
//             st = PM_ERROR;
//         }
//     }

//     if (st == PM_DONE) {
//         out_mel->data     = mel;
//         out_mel->n_frames = n_frames;
//         return 0;
//     }
//     free(mel);
//     return -1;
// }

// void mel_free(struct mel_matrix *mel) {
//     if (!mel) return;
//     free(mel->data);
//     memset(mel, 0, sizeof(*mel));
// }

/* src/pcm_to_mel.c  – Whisper 80-bin log-Mel extractor (16 kHz) */
#include "pcm_to_mel.h"
#include "audio_in.h"
#include "kissfft/kiss_fft.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define WIN_SIZE    400       /* 25 ms */
#define WIN_STRIDE  160       /* 10 ms */
#define N_FFT       512
#define N_FREQ      (N_FFT/2 + 1)   /* 257 */
#define MEL_DIM     80
#define PI_F        3.14159265358979f

/* ───────── Hanning window (預運算) ───────── */
static float hann[WIN_SIZE];
static void init_hann(void) {
    for (int i = 0; i < WIN_SIZE; ++i)
        hann[i] = 0.5f * (1.0f - cosf(2.0f*PI_F*i/(WIN_SIZE-1)));
}

/* ───────── Mel filter bank ───────── */
/* 參考 whisper.cpp build_mel_filter_bank_f32() */
static float mel_fb[MEL_DIM][N_FREQ];
static void build_mel_fb(void) {
    const float f_min =   0.0f;
    const float f_max = 8000.0f;        /* Nyquist 16 kHz */
    float mel_min = 2595.0f * log10f(1.0f + f_min/700.0f);
    float mel_max = 2595.0f * log10f(1.0f + f_max/700.0f);

    float mel_points[MEL_DIM+2];
    for (int m = 0; m < MEL_DIM+2; ++m)
        mel_points[m] = mel_min + (mel_max-mel_min)*m/(MEL_DIM+1);

    float hz_points[MEL_DIM+2];
    for (int m = 0; m < MEL_DIM+2; ++m)
        hz_points[m] = 700.0f * (powf(10.0f, mel_points[m]/2595.0f) - 1.0f);

    int bin[MEL_DIM+2];
    for (int m = 0; m < MEL_DIM+2; ++m)
        bin[m] = (int) floorf((N_FFT+1) * hz_points[m] / 16000.0f);

    memset(mel_fb, 0, sizeof(mel_fb));
    for (int m = 1; m <= MEL_DIM; ++m) {
        for (int k = bin[m-1]; k < bin[m]; ++k)
            mel_fb[m-1][k] = (float)(k - bin[m-1]) / (bin[m]-bin[m-1]);
        for (int k = bin[m]; k < bin[m+1]; ++k)
            mel_fb[m-1][k] = (float)(bin[m+1]-k) / (bin[m+1]-bin[m]);
    }
}

/* ───────── Main extractor ───────── */
int pcm_to_mel_extract(const struct pcm_buffer *pcm,
                       struct mel_matrix      *out_mel)
{
    if (!pcm || !pcm->data || pcm->sample_rate != 16000) {
        fprintf(stderr, "[mel] need 16 kHz PCM\n");
        return -1;
    }
    /* 計算 frame 數 */
    size_t total = pcm->n_samples;
    size_t n_frames = (total < WIN_SIZE) ? 0
                          : 1 + (total - WIN_SIZE)/WIN_STRIDE;
    if (n_frames == 0) { fprintf(stderr, "[mel] audio too short\n"); return -1; }

    /* malloc output */
    float *mel_out = malloc(n_frames * MEL_DIM * sizeof(float));
    if (!mel_out) return -1;

    /* 一次性初始化常量 */
    static int  inited = 0;
    static kiss_fft_cfg cfg = NULL;
    static kiss_fft_cpx fft_in[N_FFT], fft_out[N_FFT];
    static float power[N_FREQ];
    if (!inited) {
        init_hann();
        build_mel_fb();
        cfg = kiss_fft_alloc(N_FFT, 0, NULL, NULL);
        inited = 1;
    }

    /* 逐 frame 處理 */
    size_t offset = 0;
    for (size_t f = 0; f < n_frames; ++f, offset += WIN_STRIDE) {
        /* (1) window & zero-pad */
        for (int i = 0; i < WIN_SIZE; ++i) {
            fft_in[i].r = (float)pcm->data[offset+i] * (1.0f/32768.f) * hann[i];
            fft_in[i].i = 0.0f;
        }
        for (int i = WIN_SIZE; i < N_FFT; ++i)
            fft_in[i].r = fft_in[i].i = 0.0f;

        /* (2) FFT */
        kiss_fft(cfg, fft_in, fft_out);

        /* (3) power spectrum */
        for (int k = 0; k < N_FREQ; ++k)
            power[k] = fft_out[k].r*fft_out[k].r + fft_out[k].i*fft_out[k].i;

        /* (4) Mel bins */
        float *dst = mel_out + f*MEL_DIM;
        for (int m = 0; m < MEL_DIM; ++m) {
            float s = 0.0f;
            for (int k = 0; k < N_FREQ; ++k)
                s += power[k] * mel_fb[m][k];
            /* log10 + normalize (mean≈-4, std≈ 4) */
            // float v = log10f(fmaxf(s, 1e-10f));
            // dst[m] = (v + 4.0f) / 4.0f;
            float v = log10f(fmaxf(s, 1e-10f));
            float x = (v + 4.0f) / 4.0f;
            // clamp 到 [-1,1]
            if (x < -1.0f) x = -1.0f;
            else if (x >  1.0f) x =  1.0f;
            dst[m] = x;
        }
    }

    out_mel->data     = mel_out;
    out_mel->n_frames = n_frames;
    return 0;
}

void mel_free(struct mel_matrix *mel) {
    if (!mel) return;
    free(mel->data);
    memset(mel, 0, sizeof(*mel));
}