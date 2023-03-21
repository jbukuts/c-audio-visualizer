// Copyright (c) 2023 Jake Bukuts

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
// #include <libavcodec/avcodec.h>
#include <algorithm>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

using cv::VideoWriter;
using cv::Size;
using cv::Scalar;
using cv::Mat;
using cv::Point;
using cv::circle;
using cv::line;
using cv::FILLED;
using cv::LINE_AA;
using cv::LINE_8;

// defined type for unsigned char (since char8_t uses newer compiler)
typedef unsigned char uchar8_t;

const char FILE_NAME[] = "./test_files/test(orig_32_stereo).wav";
const char OUTPUT_FILE_NAME[] = "./test_32_float.mp4";
const int WIDTH = 1500;
const int HEIGHT = 1500;
const int FRAME_RATE = 60;
const int CHANNEL_SHIFT = 200;
const bool FORCE_MONO = false;
const int COLOR_TYPE = 0;
const int CIRCLE_SIZE = 2;

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

// progress helper
class progress_bar {
 private:
        int current = 1;
        float average_time = 0;
        clock_t start_time;

 public:
        int total;

        explicit progress_bar(int t) {
            // hide cursor
            printf("\e[?25l");
            total = t;
            start_time = time(NULL);
        }

        void release() {
            // show cursor
            printf("\e[?25h\n");
        }

        void increment() {
            if (current++ < total) {
                time_t elapsed = time(NULL) - start_time;
                int time_left =
                    (static_cast<float>(elapsed) / current) * (total - current);

                printf("\33[2K\r");
                printf("%.1f%% | %d / %d | %ld s elapsed (ETA: %d s)",
                    static_cast<float>(current) / total * 100,
                    current,
                    total,
                    elapsed,
                    time_left);
                fflush(stdout);
                return;
            }

            release();
        }
};

// color helper
class rainbow {
 private:
        int r = 255, g = 0, b = 0;
        int type = 0;
        Scalar* default_color = new Scalar(255, 255, 255);

 public:
        explicit rainbow(int t) {
            type = t;
            switch (t) {
                case 1:
                    default_color = new Scalar(0, 0, 255);
                    break;
                case 2:
                    default_color = new Scalar(0, 255, 0);
                    break;
                case 3:
                    default_color = new Scalar(255, 0, 0);
                    break;
                default:
                    break;
            }
        }

        Scalar get_color() {
            if (type > 0) return *default_color;

            if (r == 255 && g < 255 && b == 0)
                g++;
            else if (g == 255 && r != 0)
                r--;
            else if (r == 0 && b < 255)
                b++;
            else if (b == 255 && g != 0)
                g--;
            else if (g == 0 && r < 255)
                r++;
            else if (r == 255 & b != 0)
                b--;

            return Scalar(b, g, r);
        }
};

// populate header from file
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

// append trailing null to input
char* trail_string(uchar8_t* orig_string) {
    char *clean_string =
        reinterpret_cast<char *>(malloc(sizeof(orig_string) + 1));
    memcpy(clean_string, orig_string, sizeof(orig_string));
    clean_string[4] = '\0';
    return clean_string;
}

// helper to print header
void print_header_data(struct HEADER *header) {
    printf("WAV DATA\n");
    printf("RIFF:\t\t%s \n", trail_string(header -> riff));
    printf("SIZE:\t\t%u bytes \n", header -> overall_size);
    printf("WAVE:\t\t%s \n", trail_string(header -> wave));
    printf("FMT MARKER:\t%s \n", trail_string(header -> fmt_chunk_marker));
    printf("FMT LENGTH:\t%u \n", header -> length_of_fmt);
    printf("FORMAT TYPE:\t%u \n", header -> format_type);
    printf("CHANNELS:\t%u \n", header -> channels);
    printf("SAMPLE RATE:\t%u hz\n", header -> sample_rate);
    printf("MAGIC:\t\t%u \n", header -> magic_number);
    printf("BLOCK ALIGN:\t%u bytes\n", header -> block_align);
    printf("BIT / SAMPLE:\t%u \n", header -> bits_per_sample);
    printf("DATA:\t\t%s \n", trail_string(header -> data_chunk_header));
    printf("DATA SIZE:\t%u bytes \n", header -> data_size);
}

// read wav amplitude data from file
float_t* read_wav_data(HEADER *header, FILE *fp) {
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


int main() {
    // open file
    FILE *fp;
    fp = fopen(FILE_NAME, "rb");

    // read data into struct
    // and print metadata
    struct HEADER header = create_header(fp);
    print_header_data(&header);

    // get total file size
    fseek(fp, 0, SEEK_SET);
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    printf("FILE SIZE:\t%u bytes \n", file_size);
    fseek(fp, sizeof(header), SEEK_SET);

    // test assertions to ensure header data makes sense
    assert(sizeof(header.riff) == 4);
    assert(
        header.block_align == (header.bits_per_sample * header.channels) / 8);
    assert(header.magic_number == (header.sample_rate * header.block_align));
    assert(
        header.overall_size -
        sizeof(header) +
        sizeof(header.data_chunk_header) +
        sizeof(header.data_size) == header.data_size);
    assert(sizeof(header) == 44);

    int song_length = header.data_size / header.magic_number;
    printf("SONG LENGTH:\t%u sec \n", song_length);

    // read file data into array for parsing
    printf("\nReading WAV data!\n");
    float *wav_data = read_wav_data(&header, fp);

    // create array for canvas y values
    int *amp_data = reinterpret_cast<int *>(malloc(header.data_size));

    if (NULL == amp_data) {
        perror("malloc failed!");
        return -1;
    }

    int audio_data_length = header.data_size / header.block_align;

    // (-1,1) float values must be converted to (0,HEIGHT)
    printf("Normalizing WAV data!\n");
    for (int i=0; i < audio_data_length * header.channels; i++) {
        float_t amp_val = wav_data[i];
        amp_data[i] = ((amp_val + 1) * HEIGHT) / 2;
    }

    // all data read so close file
    assert(ftell(fp) == file_size);
    fclose(fp);

    // time to draw frames
    printf("Drawing frames!\n");
    int frames_to_draw = song_length * FRAME_RATE;
    int values_per_frame = header.sample_rate / FRAME_RATE;
    float point_spacing = static_cast<float>(WIDTH) / values_per_frame;
    rainbow color(COLOR_TYPE);

    // create video file
    Size frame_size(WIDTH, HEIGHT);
    VideoWriter ouput_video(
        OUTPUT_FILE_NAME,
        VideoWriter::fourcc('a', 'v', 'c', '1'),
        FRAME_RATE,
        frame_size,
        true);

    if (!ouput_video.isOpened())
        perror("Could not open output file");

    // actually draw frames to the video
    progress_bar progress(frames_to_draw);
    for (int i=0; i < frames_to_draw; i++) {
        // actual image to draw to
        Mat frame(frame_size, CV_8UC3, Scalar::all(0));
        Scalar new_color = color.get_color();

        // create points from audio data
        int starting_index = i * values_per_frame;
        for (int j=0; j < values_per_frame; j++) {
            int current_index = starting_index + j;
            float x_val = j * point_spacing;

            // iterate for each channel
            for (int k=0; k < header.channels; k++) {
                int shifted_index = (k*audio_data_length) + current_index;
                assert(shifted_index < audio_data_length * header.channels);
                int y_val = amp_data[shifted_index];

                Point point(x_val,  y_val);
                circle(frame, point, CIRCLE_SIZE, new_color, FILLED, LINE_AA);
            }
        }

        // write frame to file
        ouput_video << frame;
        progress.increment();
    }

    free(amp_data);

    ouput_video.release();
    return 0;
}
