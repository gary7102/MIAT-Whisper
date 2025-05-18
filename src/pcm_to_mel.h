#pragma once
#include <stddef.h>
#include <stdint.h>

struct pcm_buffer;   /* forward (audio_in.h) */

struct mel_matrix {
    float  *data;        /* size = n_frames * 80 */
    size_t  n_frames;
};

int pcm_to_mel_extract(const struct pcm_buffer *pcm,
                       struct mel_matrix      *out_mel);

void mel_free(struct mel_matrix *mel);
