#pragma once

#include "sound.h"
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <pthread.h>
#include "FFIChain.hpp"

namespace ILLIXR_AUDIO{
	class ABAudio{

	public:
		// Process types
		enum class ProcessType {
			FULL, 			// FULL for output wav file
			ENCODE, 		// For profiling, do file reading and encoding without file output
			DECODE			// For profiling, do ambisonics decoding without file output
		};
		ABAudio(std::string outputFilePath, ProcessType processType);
		// Process a block (1024) samples of sound
		void processBlock();
		// Load sound source files (predefined)
		void loadSource();

		void PrintTimeInfo(unsigned factor);

		// Buffer of most recent processed block for fast copying to audio buffer
		short mostRecentBlockL[BLOCK_SIZE];
		short mostRecentBlockR[BLOCK_SIZE];
		bool buffer_ready;

		// Number of blocks left to process before this stream is complete
		unsigned long num_blocks_left;

		// Time markers to capture time and save them
		unsigned long long StartTime;
		unsigned long long EndTime;
		unsigned long long TotalTime[N_TIME_MARKERS];

		// Helper function to start timer, end timer and
		// capture the difference in TotalTime[Index].
		void StartCounter();
		void EndCounter(unsigned Index);

		float** resultSample;

    	// BFormat file to sum up ambisonics
    	CBFormat sumBF;

		// Instance of FFT-FIR-IFFT chain object. This object is
		// eventually passed to rotator and decoder to perform the FFI
		// chains in the respective Process() functions.
		FFIChain FFIChainInst;
	private:
		ProcessType processType;
		// a list of sound sources in this audio
		std::vector<Sound> soundSrcs;
		// target output file
        std::optional<std::ofstream> outputFile;
		// decoder associated with this audio
		CAmbisonicBinauralizer decoder;
		// ambisonics rotator associated with this audio
		CAmbisonicProcessor rotator;
		// ambisonics zoomer associated with this audio
		CAmbisonicZoomer zoomer;

		int frame = 0;

		// Generate dummy WAV output file header
		void generateWAVHeader();
		// Read in data from WAV files and encode into ambisonics
		void readNEncode(CBFormat& sumBF);
		// Apply rotation and zoom effects to the ambisonics sound field
		void rotateNZoom(CBFormat& sumBF);
		// Write out a block of samples to the output file
		void writeFile(float** resultSample);

		// Abort failed configuration
		void configAbort(const std::string_view& compName) const;

		void updateRotation();
		void updateZoom();
	};
}