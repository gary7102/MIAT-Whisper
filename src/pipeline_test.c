#include <stdio.h>
#include "stub_kernels.h"   /* LoadCtx */
#include "audio_in.h"       /* AudioInput */
#include "pcm_to_mel.h"     /* Mel */

int whisper_load_ctx(const char *model,
                     struct whisper_context **out_ctx);

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <model.bin> <audio.wav>\n", argv[0]);
        return 1;
    }

    /* Step-1  LoadModelCtx (A1a) */
    struct whisper_context *ctx = NULL;
    if (whisper_load_ctx(argv[1], &ctx) != 0) {
        fprintf(stderr, "LoadCtx failed\n");
        return 1;
    }

    /* Step-2  AudioInput (A1b) */
    struct pcm_buffer pcm = {0};
    if (audio_input_load(argv[2], &pcm) != 0) {
        fprintf(stderr, "AudioInput failed\n");
        free_ctx(ctx);
        return 1;
    }

    /* Step-3  PCM→Mel (A2) */
    struct mel_matrix mel = {0};
    if (pcm_to_mel_extract(&pcm, &mel) != 0) {
        fprintf(stderr, "Mel extract failed\n");
        audio_input_free(&pcm);
        free_ctx(ctx);
        return 1;
    }

    /* --- 輸出前 5 個 Mel 值 --- */
    printf("mel.shape = (%zu, 80)\n", mel.n_frames);
    size_t dump = mel.n_frames ? 5 : 0;
    for (size_t i = 0; i < dump; ++i)
        printf("mel[0,%zu] = %.6f\n", i, mel.data[i]);
    /* 求全矩陣均值 */
    double sum = 0.0;
    for (size_t i = 0; i < mel.n_frames * 80; ++i) sum += mel.data[i];
    printf("mel.mean = %.6f\n", sum / (mel.n_frames * 80));

    /* clean-up */
    mel_free(&mel);
    audio_input_free(&pcm);
    free_ctx(ctx);
    return 0;
}
