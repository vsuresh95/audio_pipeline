#include <iostream>
#include <cassert>
#include <cstdlib>
#include "audio.h"

#ifdef ILLIXR_INTEGRATION
#include "../common/error_util.hpp"
#endif /// ILLIXR_INTEGRATION

FFIChain* FFIChainInstHandle;

extern bool can_use_audio_dma;

std::string get_path() {
#ifdef ILLIXR_INTEGRATION
    const char* AUDIO_ROOT_c_str = std::getenv("AUDIO_ROOT");
    if (!AUDIO_ROOT_c_str) {
        ILLIXR::abort("Please define AUDIO_ROOT");
    }
    std::string AUDIO_ROOT = std::string{AUDIO_ROOT_c_str};
    return AUDIO_ROOT + "samples/";
#else
    return "samples/";
#endif /// ILLIXR_INTEGRATION
}

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

    StartTime = 0;
    EndTime = 0;

	for (unsigned i = 0; i < N_TIME_MARKERS; i++)
    	TotalTime[i] = 0;

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

    resultSample = (float **) esp_alloc(2 * sizeof(float *));
    resultSample[0] = (float *) esp_alloc(BLOCK_SIZE * sizeof(float));
    resultSample[1] = (float *) esp_alloc(BLOCK_SIZE * sizeof(float));

    sumBF.Configure(NORDER, true, BLOCK_SIZE);
    
    // Hardware acceleration
    // 1. Regular invocation chain
    // 2. Shared memory invocation chain - non-pipelined or pipelined
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD || DO_FFT_IFFT_OFFLOAD) {
        // Configure accelerator parameters and write them to accelerator registers.
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);

        // Assign size parameters that is used for data store and load loops
        FFIChainInst.m_nChannelCount = rotator.m_nChannelCount;
        FFIChainInst.m_fFFTScaler = rotator.m_fFFTScaler;
        FFIChainInst.m_nBlockSize = rotator.m_nBlockSize;
        FFIChainInst.m_nFFTSize = rotator.m_nFFTSize;
        FFIChainInst.m_nFFTBins = rotator.m_nFFTBins;
        
        FFIChainInst.ConfigureAcc();

        FFIChainInst.ConfigureAcc();

    	// Write input data for psycho twiddle factors
    	FFIChainInst.InitTwiddles(&sumBF, rotator.m_pFFT_psych_cfg->super_twiddles);
    
        // For shared memory invocation cases, we can
        // start the accelerators (to start polling).
        if (DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
            FFIChainInst.StartAcc();

            // Use the DMA to load all inputs and filter weights
            // into its private scratchpad.
            if (can_use_audio_dma) {
                FFIChainInst.ConfigureDMA(rotator.m_ppcpPsychFilters, decoder.m_ppcpFilters);
            }
        }

        FFIChainInstHandle = &FFIChainInst;
    }
}


void ILLIXR_AUDIO::ABAudio::loadSource(){
#ifndef NDEBUG
    /// Temporarily clear errno here if set (until merged with #225)
    if (errno > 0) {
        errno = 0;
    }
#endif /// NDEBUG

    /// Add a bunch of sound sources
    const std::string samples_folder{get_path()};
    if (processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL) {
        soundSrcs.emplace_back(samples_folder + "lectureSample.wav", NORDER, true);
        soundSrcs.back().setSrcPos({
            .fAzimuth   = -0.1f,
            .fElevation = 3.14f/2,
            .fDistance  = 1
        });

        // soundSrcs.emplace_back(samples_folder + "radioMusicSample.wav", NORDER, true);
        // soundSrcs.back().setSrcPos({
        //     .fAzimuth   = 1.0f,
        //     .fElevation = 0.0f,
        //     .fDistance  = 5
        // });
    } else {
        for (unsigned int i = 0U; i < NUM_SRCS; i++) {

            /// This line is setting errno (whose value is 2)
            /// As a temporary fix, we set errno to 0 within Sound's constructor, and assert that
            /// it has not been set
            /// The path here is broken, we need to specify a relative path like we do in kimera
            assert(errno == 0);
            soundSrcs.emplace_back(samples_folder + "lectureSample.wav", NORDER, true);
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

    if (processType != ILLIXR_AUDIO::ABAudio::ProcessType::DECODE) {
        readNEncode(sumBF);
    }

    if (processType != ILLIXR_AUDIO::ABAudio::ProcessType::ENCODE) {
        /// Processing garbage data if just decoding
        rotateNZoom(sumBF);

        StartCounter();
        decoder.Process(&sumBF, resultSample);
        EndCounter(2);
    }

    if (processType == ILLIXR_AUDIO::ABAudio::ProcessType::FULL) {
        writeFile(resultSample);
    } 
    
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        std::cout << "  {" << std::endl << "    ";
        // std::cout << "resultSample[" << niEar << "]:" << std::endl;
        for(unsigned niSample = 0; niSample < BLOCK_SIZE; niSample++) {
            std::cout << resultSample[niEar][niSample] << ", ";
            if ((niSample + 1) % 8 == 0) std::cout << std::endl << "    ";
        }
        std::cout << std::endl << "  }," << std::endl;
    }

    // for(unsigned niEar = 0; niEar < 2; niEar++) {
    //     std::cout << "  {" << std::endl << "    ";
    //     // std::cout << "resultSample[" << niEar << "]:" << std::endl;
    //     for(unsigned niSample = 0; niSample < BLOCK_SIZE; niSample++) {
    //         std::cout << resultSample[niEar][niSample] << ", ";
    //         if ((niSample + 1) % 8 == 0) std::cout << std::endl << "    ";
    //     }
    //     std::cout << std::endl << "  }," << std::endl;
    // }

    if (num_blocks_left > 0) {
        num_blocks_left--;
    }

    // End accelerator operation with a dummy iteration
    if (num_blocks_left == 0) {
        if (DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
            FFIChainInst.EndAcc();
        }
        if (can_use_audio_dma) {
            FFIChainInst.EndDMA();
        }
    }
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
    if (DO_ROTATE_OFFLOAD) {
        FFIChainInstHandle->RotateParams.m_fCosAlpha = rotator.m_fCosAlpha;
        FFIChainInstHandle->RotateParams.m_fSinAlpha = rotator.m_fSinAlpha;
        FFIChainInstHandle->RotateParams.m_fCosBeta = rotator.m_fCosBeta;
        FFIChainInstHandle->RotateParams.m_fSinBeta = rotator.m_fSinBeta;
        FFIChainInstHandle->RotateParams.m_fCosGamma = rotator.m_fCosGamma;
        FFIChainInstHandle->RotateParams.m_fSinGamma = rotator.m_fSinGamma;
        FFIChainInstHandle->RotateParams.m_fCos2Alpha = rotator.m_fCos2Alpha;
        FFIChainInstHandle->RotateParams.m_fSin2Alpha = rotator.m_fSin2Alpha;
        FFIChainInstHandle->RotateParams.m_fCos2Beta = rotator.m_fCos2Beta;
        FFIChainInstHandle->RotateParams.m_fSin2Beta = rotator.m_fSin2Beta;
        FFIChainInstHandle->RotateParams.m_fCos2Gamma = rotator.m_fCos2Gamma;
        FFIChainInstHandle->RotateParams.m_fSin2Gamma = rotator.m_fSin2Gamma;
        FFIChainInstHandle->RotateParams.m_fCos3Alpha = rotator.m_fCos3Alpha;
        FFIChainInstHandle->RotateParams.m_fSin3Alpha = rotator.m_fSin3Alpha;
        FFIChainInstHandle->RotateParams.m_fCos3Beta = rotator.m_fCos3Beta;
        FFIChainInstHandle->RotateParams.m_fSin3Beta = rotator.m_fSin3Beta;
        FFIChainInstHandle->RotateParams.m_fCos3Gamma = rotator.m_fCos3Gamma;
        FFIChainInstHandle->RotateParams.m_fSin3Gamma = rotator.m_fSin3Gamma;

        FFIChainInstHandle->UpdateRotateParams();
    }
}


/// Simple zoom
void ILLIXR_AUDIO::ABAudio::updateZoom() {
    frame++;
    zoomer.SetZoom(sinf(frame/100));
    zoomer.Refresh();
}


/// Process some rotation and zoom effects
void ILLIXR_AUDIO::ABAudio::rotateNZoom(CBFormat& sumBF) {
    StartCounter();
    updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);
    EndCounter(0);

    StartCounter();
    updateZoom();
    zoomer.ProcessOptimized(&sumBF, BLOCK_SIZE);
    EndCounter(1);
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

void ILLIXR_AUDIO::ABAudio::StartCounter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (StartTime)
		:
		: "t0"
	);
}

void ILLIXR_AUDIO::ABAudio::EndCounter(unsigned Index) {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (EndTime)
		:
		: "t0"
	);

    TotalTime[Index] += EndTime - StartTime;
}

void ILLIXR_AUDIO::ABAudio::PrintTimeInfo(unsigned factor) {
    // uint64_t total_time = (TotalTime[0]+TotalTime[1]+TotalTime[2])/factor;
    
    // printf("\n--------------------------------------------------------------------------------------\n");
    // std::cout<<"CYCLES: "<<(total_time>1000000?(total_time/1000000):((total_time>1000)?(total_time/1000):total_time ))<<(total_time>1000000?"M":((total_time>1000)?"K":"" ))<<"\n";

    // printf("--------------------------------------------------------------------------------------\n");
    
    printf("---------------------------------------------\n");
    printf("TOP LEVEL TIME\n");
    printf("---------------------------------------------\n");
    printf("Psycho Filter\t\t = %llu\n", TotalTime[0]/factor);
    printf("Zoomer Process\t\t = %llu\n", TotalTime[1]/factor);
    printf("Binaur Filter\t\t = %llu\n", TotalTime[2]/factor);

    // Call lower-level print functions.
    rotator.PrintTimeInfo(factor);

    if (DO_FFT_IFFT_OFFLOAD || DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        FFIChainInst.PrintTimeInfo(factor, true);
    }

    decoder.PrintTimeInfo(factor);

    if (DO_FFT_IFFT_OFFLOAD || DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        FFIChainInst.PrintTimeInfo(factor, false);
    }
    printf("---------------------------------------------\n");
}

void OffloadPsychoFFTIFFT(CBFormat* pBFSrcDst, kiss_fft_cpx** m_ppcpPsychFilters, float** m_pfOverlap, unsigned m_nOverlapLength, kiss_fftr_cfg FFTcfg, kiss_fftr_cfg IFFTcfg) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    FFIChainInstHandle->PsychoRegularFFTIFFT(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap, FFTcfg, IFFTcfg);
}

void OffloadBinaurFFTIFFT(CBFormat* pBFSrc, float** ppfDst, kiss_fft_cpx*** m_ppcpFilters, float** m_pfOverlap, unsigned m_nOverlapLength, kiss_fftr_cfg FFTcfg, kiss_fftr_cfg IFFTcfg) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    FFIChainInstHandle->BinaurRegularFFTIFFT(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap, FFTcfg, IFFTcfg);
}

void OffloadPsychoChain(CBFormat* pBFSrcDst, kiss_fft_cpx** m_ppcpPsychFilters, float** m_pfOverlap, unsigned m_nOverlapLength, bool IsSharedMemory) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    if (IsSharedMemory)
        FFIChainInstHandle->PsychoNonPipelineProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    else
        FFIChainInstHandle->PsychoRegularProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
}

void OffloadBinaurChain(CBFormat* pBFSrc, float** ppfDst, kiss_fft_cpx*** m_ppcpFilters, float** m_pfOverlap, unsigned m_nOverlapLength, bool IsSharedMemory) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    if (IsSharedMemory)
        FFIChainInstHandle->BinaurNonPipelineProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
    else
        FFIChainInstHandle->BinaurRegularProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
}

void OffloadPsychoPipeline(CBFormat* pBFSrcDst, kiss_fft_cpx** m_ppcpPsychFilters, float** m_pfOverlap, unsigned m_nOverlapLength) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    if (can_use_audio_dma)
        FFIChainInstHandle->PsychoProcessDMA(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    else
        FFIChainInstHandle->PsychoProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
}

void OffloadBinaurPipeline(CBFormat* pBFSrc, float** ppfDst, kiss_fft_cpx*** m_ppcpFilters, float** m_pfOverlap, unsigned m_nOverlapLength) {
    FFIChainInstHandle->m_nOverlapLength = m_nOverlapLength;
    if (can_use_audio_dma)
        FFIChainInstHandle->BinaurProcessDMA(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
    else
        FFIChainInstHandle->BinaurProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
}

void OffloadRotateOrder(CBFormat* pBFSrcDst) {
    FFIChainInstHandle->OffloadRotateOrder(pBFSrcDst);
}
