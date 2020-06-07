# Audio Pipeline

Part of [ILLIXR](https://github.com/ILLIXR/ILLIXR), the Illinios Extended Reality testbed. The audio pipeline is responsible for both recording and playing back spatialized audio for XR.

# Setup Instructions

Clone this repo using the following command:

`git clone https://github.com/ILLIXR/audio_pipeline.git --recursive`

The audio pipeline relies upon C++17, so clang 5.0+ is required. If you are using a relatively new OS such as Ubuntu 18.04 or later, `sudo apt-get install clang` should suffice. If you're on an older OS, you can install the latest version of clang by following the steps on the [LLVM nightly packages page](https://apt.llvm.org/). If you already have clang 3.8+ and would not like to upgrade, you can replace `-std=c++17` with `-std=c++1z` in the Makefile.

Build debug: `make` or `make solo.dbg`

Build release: `make solo.opt`

**If you are switching between builds, please do `make deepclean`.**

Also note that the release build (-O3) is considerably faster than the debug build.

# Usage

`./solo.dbg <number of 1024-sample-blocks to process> [optional: decode/encode]`

1. Number of audio blocks to process is a required parameter.
2. decode/encode specifies the different audio processing steps, which are specificially designed for profiling. No output would be generated.

If encode or decode is not specified, the code will perform both encoding and decoding on a preset input sound sample file, and output spatialized audio output in `output.wav`.

## Examples:

This will generate ~11 seconds of spatialized audio using the two sound samples under `./sample/`: `./solo.dbg 500`

This will encode 2000 1024-sample-blocks of audio into HOA format: `./solo.dbg 2000 encode`

This will decode (binauralize) 2000 1024-sample-blocks of encoded HOA audio: `./solo.dbg 2000 decode`

**Note:** The input and output are hardcoded to be 48 KHz sample rate, 16-bit PCM wav files. If you want to hear the output sound, limit the number of processed sample blocks so that the output is not longer than input! Otherwise, garbage sound samples would be generated.

# Components

## libspatialaudio

Submodule libspatialaudio provides the backend library for Ambisonics encoding, decoding, rotation, zoom, and binauralization (HRTF included).

## audio pipeline code

### sound.cpp

Describes a sound source in the ambisonics sound-field, including the input file for the sound source and its position in the sound-field.

### audio.cpp

Encapsulate preset processing steps of sound source reading, encoding, rotating, zooming, and decoding.

# License

This code is available under the University of Illinois/NCSA Open Source License. The sound samples provided in `./sample/` are available under the Creative Commons 0 license.
