#include <stdio.h>
#include "audio_in.h"
#include "pcm_to_mel.h"

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s <wav>\n", argv[0]); return 1; }

    struct pcm_buffer pcm = {0};
    if (audio_input_load(argv[1], &pcm) != 0) return 1;

    struct mel_matrix mel = {0};
    if (pcm_to_mel_extract(&pcm, &mel) == 0) {
        printf("[mel] shape=(%zu, 80)\n", mel.n_frames);
        mel_free(&mel);
        audio_input_free(&pcm);
        return 0;
    }
    audio_input_free(&pcm);
    return 1;
}
