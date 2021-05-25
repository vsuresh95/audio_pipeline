#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include "sound.h"

#ifdef ILLIXR_INTEGRATION
#include "../common/error_util.hpp"
#endif /// ILLIXR_INTEGRATION


ILLIXR_AUDIO::Sound::Sound(
    std::string srcFilename,
    [[maybe_unused]] unsigned int nOrder,
    [[maybe_unused]] bool b3D
) : srcFile{srcFilename, std::fstream::in}
  , BFormat{std::make_shared<CBFormat>()}
  , amp{1.0}
{
    /// NOTE: This is currently only accepts mono channel 16-bit depth WAV file
    /// TODO: Change brutal read from wav file
    constexpr std::size_t SRC_FILE_SIZE {44U};
    std::byte temp[SRC_FILE_SIZE];
    srcFile.read(reinterpret_cast<char*>(temp), sizeof(temp));

    /// BFormat file initialization
    if (!BFormat->Configure(nOrder, true, BLOCK_SIZE)) {
        configAbort("BFormat");
    }
    BFormat->Refresh();

    /// Encoder initialization
    if (!BEncoder.Configure(nOrder, true, SAMPLERATE)) {
        configAbort("BEncoder");
    }
    BEncoder.Refresh();

    srcPos.fAzimuth   = 0.0f;
    srcPos.fElevation = 0.0f;
    srcPos.fDistance  = 0.0f;

    BEncoder.SetPosition(srcPos);
    BEncoder.Refresh();

    /// Clear errno, as this constructor is setting the flag (with value 2)
    /// A temporary fix.
    errno = 0;
}


void ILLIXR_AUDIO::Sound::setSrcPos(const PolarPoint& pos) {
    srcPos.fAzimuth   = pos.fAzimuth;
    srcPos.fElevation = pos.fElevation;
    srcPos.fDistance  = pos.fDistance;

    BEncoder.SetPosition(srcPos);
    BEncoder.Refresh();
}


void ILLIXR_AUDIO::Sound::setSrcAmp(float ampScale) {
    amp = ampScale;
}


/// TODO: Change brutal read from wav file
std::weak_ptr<CBFormat> ILLIXR_AUDIO::Sound::readInBFormat() {
    float sampleTemp[BLOCK_SIZE];
    srcFile.read((char*)sampleTemp, BLOCK_SIZE * sizeof(short));

    /// Normalize samples to -1 to 1 float, with amplitude scale
    constexpr float SAMPLE_DIV {(2 << 14) - 1}; /// 32767.0f == 2^14 - 1
    for (std::size_t i = 0U; i < BLOCK_SIZE; ++i) {
        sample[i] = amp * (sampleTemp[i] / SAMPLE_DIV);
    }

    BEncoder.Process(sample, BLOCK_SIZE, BFormat.get());
    return BFormat;
}

void ILLIXR_AUDIO::Sound::configAbort(const std::string_view& compName) const
{
    static constexpr std::string_view cfg_fail_msg{"[Sound] Failed to configure "};
#ifdef ILLIXR_INTEGRATION
    ILLIXR::abort(std::string{cfg_fail_msg} + std::string{compName});
#else
    std::cerr << cfg_fail_msg << compName << std::endl;
    std::abort();
#endif /// ILLIXR_INTEGRATION
}
