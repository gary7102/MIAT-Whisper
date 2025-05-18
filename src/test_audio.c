#include <stdio.h>
#include "audio_in.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <wav>\n", argv[0]);
        return 1;
    }
    struct pcm_buffer pcm = {0};
    if (audio_input_load(argv[1], &pcm) == 0) {
        printf("[test] frames=%zu rate=%d\n", pcm.n_samples, pcm.sample_rate);
        audio_input_free(&pcm);
        return 0;
    }
    return 1;
}
