#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <spatialaudio/Ambisonics.h>

constexpr std::size_t SAMPLERATE {48000U};
constexpr std::size_t BLOCK_SIZE {512U};
constexpr std::size_t NORDER     {3U};
constexpr std::size_t NUM_SRCS   {16U};

#define NUM_CHANNELS (OrderToComponents(NORDER, true))

namespace ILLIXR_AUDIO{
	class Sound{
	public:
		Sound(std::string srcFile, unsigned nOrder, bool b3D);
		// set sound src position
		void setSrcPos(const PolarPoint& pos);
		// set sound amplitude scale
		void setSrcAmp(float ampScale);
		// read sound samples from mono 16bit WAV file and encode into ambisonics format
        std::weak_ptr<CBFormat> readInBFormat();
	private:
		// corresponding sound src file
		std::fstream srcFile;
		// sample buffer HARDCODE
		float sample[BLOCK_SIZE];
		// ambisonics format sound buffer
        std::shared_ptr<CBFormat> BFormat;
		// ambisonics encoder, containing format info, position info, etc.
		CAmbisonicEncoderDist BEncoder;
		// ambisonics position
		PolarPoint srcPos;
		// amplitude scale to avoid clipping
		float amp;

		// Abort failed configuration
		void configAbort(const std::string_view& compName) const;
	};
}
