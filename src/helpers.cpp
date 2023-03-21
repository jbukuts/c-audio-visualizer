// Copyright (c) 2023 Jake Bukuts

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/helpers.h"

using helpers::progress_bar;
using helpers::rainbow;

// progress_bar constructor
progress_bar::progress_bar(int t) {
    printf("\e[?25l");
    total = t;
    start_time = time(NULL);
}

// returns the cursor at end
void progress_bar::release() {
    printf("\e[?25h\n");
}

// increments the progress bar
void progress_bar::increment() {
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

// rainbow constructor
rainbow::rainbow(int t) {
    type = t;
    switch (t) {
        case 1:
            default_color[0] = 255;
            break;
        case 2:
            default_color[1] = 255;
            break;
        case 3:
            default_color[2] = 255;
            break;
        default:
            break;
    }
}

// returns incremented color as needed
int *rainbow::get_color() {
    int *c = reinterpret_cast<int*>(malloc(3*sizeof(int)));
    if (type > 0) {
        c[0] = default_color[0];
        c[1] = default_color[1];
        c[2] = default_color[2];
        return c;
    }

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

    c[0] = b;
    c[1]= g;
    c[2] = r;
    return c;
}
