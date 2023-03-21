# C++ Audio Visualizer

Amplitude audio visualizer written in C++

## Starting locally

In order to build this project locally you'll need to have both OpenCV and CMake installed. I opted to build it from source from the [repo](https://github.com/opencv/opencv).

> Note: For OpenCV to be able to encode video you'll need to ensure when installing from source the `FFMPEG` option is set as true. 
If not you may need to install some additional packages.


## Use

Audio is not muxed on the creation of the video currently. For now, I've opted to use the `ffmpeg` CLI to add audio with a command like so:

```bash
./main && ffmpeg -i [IN_VID] -i [WAV_FILE] -map 0:v -map 1:a -c:v copy [OUT_VID]
```

## Support

This project uses a custom WAV parser so support is not 100% for all WAV encodings. Support for all bit-rates as well as stereo/mono should be solid. The real issues is the variable length of amplitude data. Below I've created a table listing the encodings I've tested:

| Format | 

Also, only formats 1 and 3 are supported. For format 3 only 32-bit floating point values are allowed. I may extend support for 64-bit in the future. 