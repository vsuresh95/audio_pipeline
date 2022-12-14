# Audio Pipeline

Part of [ILLIXR](https://github.com/ILLIXR/ILLIXR), the Illinios Extended Reality testbed. The audio pipeline is responsible for both recording and playing back spatialized audio for XR. This is a simplified bare metal version of the original app.

## Setup Instructions

This repo is included as a submodule of [ESP](https://github.com/vsuresh95/esp).

Or you could clone this repo standalone using the following command:

`git clone git@github.com:vsuresh95/audio_pipeline.git`

The build commands for audio-pipeline are defined in `<ESP_ROOT>/utils/make/audio_pipeline.mk` and described below.

## Usage
All commands need to be run from the build folder in ESP:
```
cd socs/<FPGA Board Folder>
```

To build the audio-pipeline in ESP:
```
make audio-pipeline
```

To run the audio-pipeline in ESP using RTL simulation:
```
make xmsim-gui-audio-pipeline
```

To run the audio-pipeline in ESP using FPGA:
```
make fpga-program fpga-run-audio-pipeline
```

To clean:
```
make audio-pipeline-clean
```

## Custom options for make
- `NUM_BLOCKS`: Number of audio blocks (16)
- `BLOCK_SIZE`: Number of samples in each audio (1024)
- `SAMPLERATE`: Unused
- `NORDER`: Order of audio decoding (3)
- `NUM_SRCS`: Number of sources, or channel of audio (16)
- `COH_MODE`: Specialization of ESP/SPX coherence protocol (0).
- `IS_ESP`: Use ESP caches or Spandex caches (1)
- `DO_CHAIN_OFFLOAD`: Offload FFT-FIR-IFFT chain to accelerators, with regular invocation (0)
- `DO_NP_CHAIN_OFFLOAD`: Offload FFT-FIR-IFFT chain to accelerators, with SM invocation (0)
- `USE_INT`: Use int type for all data, or float for CPU data and fixed point for accelerators (1)
