// Copyright (c) 2023 Jake Bukuts

#ifndef HELPERS_H
#define HELPERS_H

namespace helpers {
    class progress_bar {
     private:
        int current = 1;
        float average_time = 0;
        clock_t start_time;

     public:
        int total;

        progress_bar(int t);

        void release();

        void increment();
    };

    class rainbow {
     private:
        int r = 255, g = 0, b = 0;
        int type = 0;
        int default_color[3] = {0,0,0};

     public:
        rainbow(int t);

        int *get_color();
    };
}

#endif