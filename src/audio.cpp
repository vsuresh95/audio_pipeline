#include <iostream>
#include <cassert>
#include <cstdlib>
#include "audio.h"

#ifdef ILLIXR_INTEGRATION
#include "../common/error_util.hpp"
#endif /// ILLIXR_INTEGRATION


ILLIXR_AUDIO::ABAudio::ABAudio(std::string outputFilePath, ProcessType procTypeIn)
    : processType {procTypeIn}
    , outputFile {
          processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL
          ? std::make_optional<std::ofstream>(outputFilePath, std::ios_base::out | std::ios_base::binary)
          : std::nullopt
      }
{
    if (processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL) {
        generateWAVHeader();
    }

    unsigned int tailLength {0U};

    /// Binauralizer as ambisonics decoder
    if (!decoder.Configure(NORDER, true, SAMPLERATE, BLOCK_SIZE, tailLength)) {
        configAbort("decoder");
    }

    /// Processor to rotate
    if (!rotator.Configure(NORDER, true, BLOCK_SIZE, tailLength)) {
        configAbort("rotator");
    }

    /// Processor to zoom
    if (!zoomer.Configure(NORDER, true, tailLength)) {
        configAbort("zoomer");
    }

    buffer_ready = false;
    num_blocks_left = 0;
}


void ILLIXR_AUDIO::ABAudio::loadSource(){
#ifndef NDEBUG
    /// Temporarily clear errno here if set (until merged with #225)
    if (errno > 0) {
        errno = 0;
    }
#endif /// NDEBUG

    /// Add a bunch of sound sources
    if (processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL) {
        soundSrcs.emplace_back("samples/lectureSample.wav", NORDER, true);
        soundSrcs.back().setSrcPos({
            .fAzimuth   = -0.1f,
            .fElevation = 3.14f/2,
            .fDistance  = 1
        });

        soundSrcs.emplace_back("samples/radioMusicSample.wav", NORDER, true);
        soundSrcs.back().setSrcPos({
            .fAzimuth   = 1.0f,
            .fElevation = 0.0f,
            .fDistance  = 5
        });
    } else {
        for (unsigned int i = 0U; i < NUM_SRCS; i++) {

            /// This line is setting errno (whose value is 2)
            /// As a temporary fix, we set errno to 0 within Sound's constructor, and assert that
            /// it has not been set
            /// The path here is broken, we need to specify a relative path like we do in kimera
            assert(errno == 0);
            soundSrcs.emplace_back("samples/lectureSample.wav", NORDER, true);
            assert(errno == 0);

            soundSrcs.back().setSrcPos({
                .fAzimuth   = i * -0.1f,
                .fElevation = i * 3.14f/2,
                .fDistance  = i * 1.0f
            });
        }
    }
}


void ILLIXR_AUDIO::ABAudio::processBlock() {
    float** resultSample = new float*[2];
    resultSample[0] = new float[BLOCK_SIZE];
    resultSample[1] = new float[BLOCK_SIZE];

    /// Temporary BFormat file to sum up ambisonics
    CBFormat sumBF;
    sumBF.Configure(NORDER, true, BLOCK_SIZE);

    if (processType != ILLIXR_AUDIO::ABAudio::ProcessType::DECODE) {
        readNEncode(sumBF);
    }

    if (processType != ILLIXR_AUDIO::ABAudio::ProcessType::ENCODE) {
        /// Processing garbage data if just decoding
        rotateNZoom(sumBF);
        decoder.Process(&sumBF, resultSample);
    }

    if (processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL) {
        writeFile(resultSample);
        if (num_blocks_left > 0) {
            num_blocks_left--;
        }
    }

    delete[] resultSample[0];
    delete[] resultSample[1];
    delete[] resultSample;
}


/// Read from WAV files and encode into ambisonics format
void ILLIXR_AUDIO::ABAudio::readNEncode(CBFormat& sumBF) {
    for (unsigned int soundIdx = 0U; soundIdx < soundSrcs.size(); ++soundIdx) {
        /// 'readInBFormat' now returns a weak_ptr, ensuring that we don't access
        /// or destruct a freed resource
        std::weak_ptr<CBFormat> tempBF_weak {soundSrcs[soundIdx].readInBFormat()};
        std::shared_ptr<CBFormat> tempBF{tempBF_weak.lock()};

        if (tempBF != nullptr) {
            if (soundIdx == 0U) {
                sumBF = *tempBF;
            } else {
                sumBF += *tempBF;
            }
        } else {
            static constexpr std::string_view read_fail_msg{
                "[ABAudio] Failed to read/encode. Sound has expired or been destroyed."
            };
#ifdef ILLIXR_INTEGRATION
            ILLIXR::abort(std::string{read_fail_msg});
#else
            std::cerr << read_fail_msg << std::endl;
            std::abort();
#endif /// ILLIXR_INTEGRATION
        }
   }
}


/// Simple rotation
void ILLIXR_AUDIO::ABAudio::updateRotation() {
    frame++;
    Orientation head(0,0,1.0*frame/1500*3.14*2);
    rotator.SetOrientation(head);
    rotator.Refresh();
}


/// Simple zoom
void ILLIXR_AUDIO::ABAudio::updateZoom() {
    frame++;
    zoomer.SetZoom(sinf(frame/100));
    zoomer.Refresh();
}


/// Process some rotation and zoom effects
void ILLIXR_AUDIO::ABAudio::rotateNZoom(CBFormat& sumBF) {
    updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);
    updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);
}


void ILLIXR_AUDIO::ABAudio::writeFile(float** resultSample) {
    /// Normalize(Clipping), then write into file
    for (std::size_t sampleIdx = 0U; sampleIdx < BLOCK_SIZE; ++sampleIdx) {
        resultSample[0][sampleIdx] = std::max(std::min(resultSample[0][sampleIdx], +1.0f), -1.0f);
        resultSample[1][sampleIdx] = std::max(std::min(resultSample[1][sampleIdx], +1.0f), -1.0f);
        int16_t tempSample0 = (int16_t)(resultSample[0][sampleIdx]/1.0 * 32767);
        int16_t tempSample1 = (int16_t)(resultSample[1][sampleIdx]/1.0 * 32767);
        outputFile->write((char*)&tempSample0,sizeof(short));
        outputFile->write((char*)&tempSample1,sizeof(short));

        /// Cache written block in object buffer until needed by realtime audio thread
        mostRecentBlockL[sampleIdx] = tempSample0;
        mostRecentBlockR[sampleIdx] = tempSample1;
    }
}


namespace ILLIXR_AUDIO
{
    /// NOTE: WAV FILE SIZE is not correct
    typedef struct __attribute__ ((packed)) WAVHeader_t
    {
        unsigned int sGroupID = 0x46464952;
        unsigned int dwFileLength = 48000000;       /// A large enough random number
        unsigned int sRiffType = 0x45564157;
        unsigned int subchunkID = 0x20746d66;
        unsigned int subchunksize = 16;
        unsigned short audioFormat = 1;
        unsigned short NumChannels = 2;
        unsigned int SampleRate = 48000;
        unsigned int byteRate = 48000*2*2;
        unsigned short BlockAlign = 2*2;
        unsigned short BitsPerSample = 16;
        unsigned int dataChunkID = 0x61746164;
        unsigned int dataChunkSize = 48000000;      /// A large enough random number
    } WAVHeader;
}


void ILLIXR_AUDIO::ABAudio::generateWAVHeader() {
    /// Brute force wav header
    WAVHeader wavh;
    outputFile->write((char*)&wavh, sizeof(WAVHeader));
}


void ILLIXR_AUDIO::ABAudio::configAbort(const std::string_view& compName) const
{
    static constexpr std::string_view cfg_fail_msg{"[ABAudio] Failed to configure "};
#ifdef ILLIXR_INTEGRATION
    ILLIXR::abort(std::string{cfg_fail_msg} + std::string{compName});
#else
    std::cerr << cfg_fail_msg << compName << std::endl;
    std::abort();
#endif /// ILLIXR_INTEGRATION
}
