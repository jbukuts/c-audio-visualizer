// Copyright (c) 2023 Jake Bukuts

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <algorithm>

#include "wavparse.h"

using namespace wavparse;

using wavparse::HEADER;

typedef unsigned char uchar8_t;

// create the header data
struct HEADER create_header(FILE *fp) {
    // to the beginning
    fseek(fp, 0, SEEK_SET);

    struct HEADER header;
    fread(header.riff, 4, 1, fp);
    fread(&(header.overall_size), sizeof(header.overall_size), 1, fp);
    fread(header.wave, 4, 1, fp);
    fread(header.fmt_chunk_marker, 4, 1, fp);
    fread(&(header.length_of_fmt), 4, 1, fp);
    fread(&(header.format_type), 2, 1, fp);
    fread(&(header.channels), 2, 1, fp);
    fread(&(header.sample_rate), 4, 1, fp);
    fread(&(header.magic_number), 4, 1, fp);
    fread(&(header.block_align), 2, 1, fp);
    fread(&(header.bits_per_sample), 2, 1, fp);
    fread(header.data_chunk_header, 4, 1, fp);
    fread(&(header.data_size), 4, 1, fp);
    return header;
}

// actually parses wave data from the binary
float_t* read_wav_data(struct HEADER *header, FILE *fp) {
    // from header
    const uint16_t format_type = header -> format_type;
    const uint16_t bits_per_sample = header -> bits_per_sample;
    const uint16_t block_align = header -> block_align;
    const uint32_t data_size = header -> data_size;
    const uint16_t channels = header -> channels;

    // calculated values
    const uint16_t bytes_per_sample = bits_per_sample / 8;
    const uint64_t max_value = static_cast<uint64_t>(pow(2, bits_per_sample));
    const int audio_data_length = data_size / block_align;

    auto log_amp_val = [](float_t amp_val) {
        return (amp_val < 0 ? -1.0 : 1.0) * (log10 (abs(amp_val) + 0.1) + 1);
    };

    printf("SIZE: %d\n", audio_data_length * channels);
    printf("MAX: %ld\n", max_value);

    // stuff we'll fill in
    float_t *wav_data = reinterpret_cast<float_t *>
        (malloc(audio_data_length * channels * sizeof(float_t)));
    float_t amp_val = 0.0;
    int32_t pcm_amp_val = 0;
    for (int i=0; i < audio_data_length; i++) {
        // iterate over channels
        for (int j=0; j < channels; j++) {
            // read in data as float
            fread(reinterpret_cast<char *>(&amp_val), bytes_per_sample, 1, fp);

            int current_index = (j*audio_data_length) + i;

            if (format_type == 3) {
                wav_data[current_index] = (float_t) amp_val;
            } else if (format_type == 1) {
                // evil floating point bit level hacking
                memcpy(&pcm_amp_val, &amp_val, bytes_per_sample);
                float_t val = ((float_t) (pcm_amp_val * 2) / max_value) - 1.0;

                if (bytes_per_sample > 1) {
                    if (val < 0) val = val + 1.0;
                    else if (val > 0) val = val - 1.0;
                }
                                
                wav_data[current_index] = val;
            }
            wav_data[current_index] = log_amp_val(wav_data[current_index]);
        }
    }

    printf("Done reading WAV data!\n");
    return wav_data;
}

// constructor
parse_wav::parse_wav(FILE *fp) {
    // create the header and parse data
    header = create_header(fp);
    wav_data = read_wav_data(&header, fp);
}

// contructor for file path
parse_wav::parse_wav(char* file_path) {
    // open file
    FILE *fp;
    fp = fopen(file_path, "rb");

    parse_wav((FILE *) fp);
}

// prints the header data
void parse_wav::print_header() {
    // help to append trailing null
    auto trail_string = [](uchar8_t* orig_string) {
        char *clean_string =
            reinterpret_cast<char *>(malloc(sizeof(orig_string) + 1));
        memcpy(clean_string, orig_string, sizeof(orig_string));
        clean_string[4] = '\0';
        return clean_string;
    };

    printf("WAV DATA\n");
    printf("RIFF:\t\t%s \n", trail_string(header.riff));
    printf("SIZE:\t\t%u bytes \n", header.overall_size);
    printf("WAVE:\t\t%s \n", trail_string(header.wave));
    printf("FMT MARKER:\t%s \n", trail_string(header.fmt_chunk_marker));
    printf("FMT LENGTH:\t%u \n", header.length_of_fmt);
    printf("FORMAT TYPE:\t%u \n", header.format_type);
    printf("CHANNELS:\t%u \n", header.channels);
    printf("SAMPLE RATE:\t%u hz\n", header.sample_rate);
    printf("MAGIC:\t\t%u \n", header.magic_number);
    printf("BLOCK ALIGN:\t%u bytes\n", header.block_align);
    printf("BIT / SAMPLE:\t%u \n", header.bits_per_sample);
    printf("DATA:\t\t%s \n", trail_string(header.data_chunk_header));
    printf("DATA SIZE:\t%u bytes \n", header.data_size);
}

