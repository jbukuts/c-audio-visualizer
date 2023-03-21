// Copyright (c) 2023 Jake Bukuts

#ifndef WAVPARSE_H
#define WAVPARSE_H

namespace wavparse {

    struct HEADER {
        uchar8_t riff[4];
        uint32_t overall_size;
        uchar8_t wave[4];
        uchar8_t fmt_chunk_marker[4];
        uint32_t length_of_fmt;
        uint16_t format_type;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t magic_number;
        uint16_t block_align;
        uint16_t bits_per_sample;
        uchar8_t data_chunk_header[4];
        uint32_t data_size;
    };

    class parse_wav {
     public:
        struct HEADER header;
        float_t* wav_data;

        // constructor
        parse_wav(FILE *fp);

        // constructor for file path
        parse_wav(char* file_path);

        void print_header();
    };
}

#endif