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

struct HEADER {
    unsigned char riff[4];
    unsigned int overall_size;
    unsigned char wave[4];
    unsigned char fmt_chunk_marker[4];
    unsigned int length_of_fmt;
    uint16_t format_type;
    uint16_t channels;
    unsigned int sample_rate;
    unsigned int magic_number;
    uint16_t block_align;
    uint16_t bits_per_sample;
    unsigned char data_chunk_header[4];
    unsigned int data_size;
};

const char FILE_NAME[] = "./FREE.wav";
const char OUTPUT_FILE_NAME[] = "./test.mp4";
const int WIDTH = 1500;
const int HEIGHT = 1500;
const int FRAME_RATE = 60;
const float AMP_SCALE = 1.35;
const int CHANNEL_SHIFT = 200;
const bool FORCE_MONO = false;
const int COLOR_TYPE = 0;
const int CIRCLE_SIZE = 2;

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

char* trail_string(unsigned char* orig_string) {
    char *clean_string =
        reinterpret_cast<char *>(malloc(sizeof(orig_string) + 1));
    memcpy(clean_string, orig_string, sizeof(orig_string));
    clean_string[4] = '\0';
    return clean_string;
}

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

// determine the type of the wav byte data
auto determine_type() {
    return (float_t) 1.0;
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
        header.magic_number ==
        (header.sample_rate * header.bits_per_sample * header.channels) / 8);
    assert(
        header.block_align == (header.bits_per_sample * header.channels) / 8);
    assert(
        header.overall_size -
        sizeof(header) +
        sizeof(header.data_chunk_header) +
        sizeof(header.data_size) == header.data_size);
    assert(
        (header.bits_per_sample * header.channels) / 8 == header.block_align);
    assert(sizeof(header) == 44);

    int song_length = header.data_size / header.magic_number;
    printf("SONG LENGTH:\t%u sec \n", song_length);

    if (header.channels != 2||
        header.block_align != 8 ||
        header.format_type != 3) {
        printf(
            "File must be:\nCHANNELS: 2\nBLOCK ALIGN: 8\nFORMAT TYPE: 3\n");
        return -1;
    }

    // read file data into array for parsing
    printf("\nReading WAV data!\n");

    // create dynamically sized array for byte data
    int *left_channel = reinterpret_cast<int *>(malloc(header.data_size >> 1));
    int *right_channel = reinterpret_cast<int *>(malloc(header.data_size >> 1));

    if (NULL == left_channel || NULL == right_channel) {
        perror("malloc failed!");
        return -1;
    }

    assert((header.data_size >> 1) / sizeof(int) ==
        header.data_size / header.block_align);

    // (-1,1) float values must be converted to (0,HEIGHT)
    int audio_data_length = header.data_size / header.block_align;
    auto determined_type = determine_type();
    decltype(determined_type) amp_value;
    printf("Normalizing WAV data!\n");
    for (int i=0; i < audio_data_length; i++) {
        // read audio data from file
        fread(&amp_value, sizeof(amp_value), 1, fp);
        left_channel[i] =
            HEIGHT - (((amp_value * AMP_SCALE + 1) * HEIGHT) / 2);

        fread(&amp_value, sizeof(amp_value), 1, fp);
        right_channel[i] =
            HEIGHT - (((amp_value * AMP_SCALE + 1) * HEIGHT) / 2);
    }

    // all data read so close file
    assert(ftell(fp) == file_size);
    fclose(fp);

    // time to draw frames
    printf("Drawing frames!\n");
    int frames_to_draw = song_length * FRAME_RATE;
    int values_per_frame = header.sample_rate / FRAME_RATE;
    size_t frame_byte_size = values_per_frame * sizeof(*left_channel);
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

        // list of points for frame
        int point_list_size = values_per_frame * sizeof(Point);
        Point *frame_points_left =
            reinterpret_cast<Point *>(malloc(point_list_size));
        Point *frame_points_right =
            reinterpret_cast<Point *>(malloc(point_list_size));

        Scalar new_color = color.get_color();

        // create points from audio data
        int starting_index = i * values_per_frame;
        for (int j=0; j < values_per_frame; j++) {
            int current_index = starting_index + j;
            float x_val = j * point_spacing;

            int left_y_val = left_channel[current_index];
            int right_y_val = right_channel[current_index];

            // create left and right channel points
            Point left_point(
                x_val,  left_y_val - CHANNEL_SHIFT);
            Point right_point(
                x_val, right_y_val + CHANNEL_SHIFT);

            frame_points_left[j] = left_point;
            frame_points_right[j] = right_point;

            // if mono sum/average, draw and early continue
            if (FORCE_MONO == true) {
                float mono_y_val =
                    static_cast<float>(left_y_val +right_y_val) / 2;
                float center_y = HEIGHT / 2;
                float inverse_add = center_y - mono_y_val;

                Point point_a(x_val, mono_y_val);
                Point point_b(x_val, center_y + inverse_add);

                line(
                    frame,
                    point_a, point_b, new_color, CIRCLE_SIZE, LINE_AA);
                continue;
            }

            circle(frame, left_point, CIRCLE_SIZE, new_color, FILLED, LINE_AA);
            circle(frame, right_point, CIRCLE_SIZE, new_color, FILLED, LINE_AA);
        }

        // drop the line into the frame
        // polylines(
        // frame, &frame_points_left,
        // &values_per_frame, 0, false, color, 2, LINE_8, 0);

        // write frame to file
        ouput_video << frame;
        progress.increment();

        free(frame_points_left);
        free(frame_points_right);
    }

    free(left_channel);
    free(right_channel);

    ouput_video.release();
    return 0;
}
