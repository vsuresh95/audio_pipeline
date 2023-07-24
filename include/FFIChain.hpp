#ifndef FFICHAIN_H
#define FFICHAIN_H

// #define USE_INT

// Use float for CPU data and fixed point for accelerators
typedef float audio_t;
typedef int device_t;

#include <CohDefines.hpp>

#include <_kiss_fft_guts.h>
#include <kiss_fftr.h>
#include <BFormat.h>

// We need to convert from float to fixed, and vice-versa,
// if we are using float for the CPU data, and fixed for
// accelerator data. We use a precision of 4 integer bits
// for the fixed-point data (defined in accelerator as well).
#if (USE_INT == 1)
#define FLOAT_TO_FIXED_WRAP(x, y) x
#define FIXED_TO_FLOAT_WRAP(x, y) x
#else
#include <fixed_point.h>
#define AUDIO_FX_IL 14
#define ROTATE_FX_IL 16
#define FLOAT_TO_FIXED_WRAP(x, y) float_to_fixed32(x, y)
#define FIXED_TO_FLOAT_WRAP(x, y) fixed32_to_float(x, y)
#endif

#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

// Control partition layout
// 0 - [From Producer] Valid Flag
// 1 - Padding
// 2 - [From Producer] End Flag
// 3 - Padding
// 4 - [To Producer] Ready Flag
// 5 - Padding
// 6 - [FIR only] [From Producer] Fitler Valid Flag
// 7 - Padding
// 8 - [FIR only] [To Producer] Fitler Ready Flag
// 9 - Padding
#define SYNC_VAR_SIZE 10
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define END_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4
#define FLT_VALID_FLAG_OFFSET 6
#define FLT_READY_FLAG_OFFSET 8
#define NUM_DEVICES 3

#define LOAD_STORE_FLAG_OFFSET 2
#define NUM_CFG_REG 8

#define RD_SIZE SYNC_VAR_SIZE
#define RD_SP_OFFSET SYNC_VAR_SIZE + 1
#define MEM_SRC_OFFSET SYNC_VAR_SIZE + 2
#define WR_SIZE SYNC_VAR_SIZE + 3
#define WR_SP_OFFSET SYNC_VAR_SIZE + 4
#define MEM_DST_OFFSET SYNC_VAR_SIZE + 5
#define CONS_VALID_OFFSET SYNC_VAR_SIZE + 6
#define CONS_READY_OFFSET SYNC_VAR_SIZE + 7

struct rotate_params_t {
    float m_fCosAlpha;
    float m_fSinAlpha;
    float m_fCosBeta;
    float m_fSinBeta;
    float m_fCosGamma;
    float m_fSinGamma;
    float m_fCos2Alpha;
    float m_fSin2Alpha;
    float m_fCos2Beta;
    float m_fSin2Beta;
    float m_fCos2Gamma;
    float m_fSin2Gamma;
    float m_fCos3Alpha;
    float m_fSin3Alpha;
    float m_fCos3Beta;
    float m_fSin3Beta;
    float m_fCos3Gamma;
    float m_fSin3Gamma;
};

class FFIChain {
public:
    // Buffer pointers.
    device_t *mem;
    volatile device_t* sm_sync;

    // Extra buffer for FIR in SW
    kiss_fft_cpx* freqdata;

    // Accelerator parameters.
    unsigned logn_samples;
    unsigned num_samples;
    unsigned in_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned mem_size;
    unsigned acc_len;
    unsigned acc_size;
    unsigned dma_offset;
     
    spandex_config_t SpandexConfig;
    accelerator_coherence CoherenceMode;

    // Sync flag offsets.
	unsigned ConsRdyFlag;
	unsigned ConsVldFlag;
	unsigned FltRdyFlag;
	unsigned FltVldFlag;
	unsigned ProdRdyFlag;
	unsigned ProdVldFlag;
	unsigned DMARdyFlag;
	unsigned DMAVldFlag;

    // Data size parameters. These are set from the rotator's
    // parameters in the configure phase.
    unsigned m_nChannelCount;
    unsigned m_nBlockSize;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    unsigned m_nOverlapLength;
    audio_t m_fFFTScaler;

    // Scratchpad offsets
    unsigned init_data_offset;
    unsigned psycho_filters_offset;
    
    // Time markers to capture time and save them
    unsigned long long StartTime;
    unsigned long long EndTime;
    unsigned long long TotalTime[N_TIME_MARKERS];

    rotate_params_t RotateParams;

    // Helper function to start timer, end timer and
    // capture the difference in TotalTime[Index].
    void StartCounter();
    void EndCounter(unsigned Index);

    void SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl);
    void SetCohMode(unsigned UseESP, unsigned CohPrtcl);

    void InitParams();
    void ConfigureAcc();
    void StartAcc();
    void EndAcc();
    void StartDMA();
    void EndDMA();

    // Set the default sync flags offsets for all accelerators
    void InitSyncFlags();

    // Chain offload using regular accelerator invocation.
    // Here, pBFSrcDst is the shared BFormat passed between
    // tasks in the application.
    // m_Filters is a pointer to array containing the filters
    // for THIS CHANNEL ONLY.
    // m_pfScratchBufferA is the output buffer.
    // CurChannel is the current channel between operated on.
    void PsychoRegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap);
    void BinaurRegularProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap);

    // Chain offload using shared memory accelerator invocation.
    // Parameter definitions same as above.
    void PsychoNonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap);
    void BinaurNonPipelineProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap);

    // Copy twiddles factors (with format conversion) from
    // super_twiddles to FIR's twiddle buffer.
    void InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles);

    // Copy input data for FFT (with format conversion) from
    // pBFSrcDst->m_ppfChannels to FFT's input buffer.
    void InitData(CBFormat* pBFSrcDst, unsigned niChannel, bool IsInit);

    // Copy FIR filters (with format conversion) from
    // m_Filters to FIR's filter buffer.
    void InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters);

    // Copy IFFT output (with format conversion) from
    // IFFT's output buffer to m_pfScratchBufferA.
    void ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA);

    // In case of pipelined operation, this function
    // replaces ReadOutput. It performs ReadOutput,
    // as well as the overlap operation.
    void PsychoOverlap(CBFormat* pBFSrcDst, audio_t** m_pfOverlap, unsigned CurChannel);

    // Handle pipelined operation of psycho-acoustic filter.
    void PsychoProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap);

    // In case of pipelined operation, this function
    // replaces ReadOutput. It performs ReadOutput,
    // as well as the overlap operation.
    // void BinaurOverlap(CBFormat* pBFSrcDst, audio_t* ppfDst, audio_t* m_pfOverlap, bool isLast, bool isFirst);
    void BinaurOverlap(CBFormat* pBFSrcDst, audio_t* ppfDst, audio_t* m_pfOverlap, bool isLast, bool isFirst, unsigned CurChannel = 0);

    // Handle pipelined operation of binauralizer filter.
    void BinaurProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap);

    void PrintTimeInfo(unsigned factor, bool isPsycho = true);

    // Configure class parameters of the DMA.
    void ConfigureDMA(kiss_fft_cpx** m_ppcpPsychFilters, kiss_fft_cpx*** m_ppcpFilters);

    // Handle pipelined operation of psycho-acoustic filter, with DMA.
    void PsychoProcessDMA(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap);

    // Handle pipelined operation of binauralizer filter, with DMA.
    void BinaurProcessDMA(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap);

    // Pre-load all input and filter data to the audio DMA scratchpad.
    void LoadAllData(kiss_fft_cpx** m_ppcpPsychFilters, kiss_fft_cpx*** m_ppcpFilters);

    // Store input data from audio DMA scratchpad to FFT input buffer on demand.
    void StoreInputData(unsigned InitChannel);

    // Store psycho-acoustic filters from audio DMA scratchpad to FIR filter buffer on demand.
    void StorePsychoFilters(unsigned InitChannel);
    
    // Store binauralizer filters from audio DMA scratchpad to FIR filter buffer on demand.
    void StoreBinaurFilters(unsigned InitEar, unsigned InitChannel);

    // Offload only FFT and IFFT to accelerators
    void PsychoRegularFFTIFFT(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap, kiss_fftr_cfg FFTcfg, kiss_fftr_cfg IFFTcfg);
    void BinaurRegularFFTIFFT(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap, kiss_fftr_cfg FFTcfg, kiss_fftr_cfg IFFTcfg);
    void FIR_SW(kiss_fftr_cfg FFTcfg, kiss_fft_cpx* m_Filters, kiss_fftr_cfg IFFTcfg);

    // Generic APIs for interfacing with ASI sync flags
    void UpdateSync(unsigned FlagOFfset, int64_t value);
    void SpinSync(unsigned FlagOFfset, int64_t value);
    bool TestSync(unsigned FlagOFfset, int64_t value);

    // Accelerate rotate order.
    void OffloadRotateOrder(CBFormat* pBFSrcDst);
    void UpdateRotateParams();
};

#endif // FFICHAIN_H
