#ifndef FFICHAIN_H
#define FFICHAIN_H

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FFTAcc.hpp>
#include <FIRAcc.hpp>

#include <kiss_fftr.hpp>
#include <BFormat.hpp>

#include <AudioBase.hpp>

// We need to convert from float to fixed, and vice-versa,
// if we are using float for the CPU data, and fixed for
// accelerator data. We use a precision of 4 integer bits
// for the fixed-point data (defined in accelerator as well).
#if (USE_INT == 1)
#define FLOAT_TO_FIXED_WRAP(x, y) x
#define FIXED_TO_FLOAT_WRAP(x, y) x
#else
#include <fixed_point.h>
#define AUDIO_FX_IL 4
#define FLOAT_TO_FIXED_WRAP(x, y) float_to_fixed32(x, y)
#define FIXED_TO_FLOAT_WRAP(x, y) fixed32_to_float(x, y)
#endif

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

class FFIChain : public AudioBase {
public:
    // Accelerator objects.
    FFTAcc FFTInst;
    FIRAcc FIRInst;
    FFTAcc IFFTInst;

    // Buffer pointers.
    device_t*mem;
    unsigned **ptable;
    volatile device_t* sm_sync;

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
     
    spandex_config_t SpandexConfig;
    unsigned CoherenceMode;

    // Sync flag offsets.
	unsigned ConsRdyFlag;
	unsigned ConsVldFlag;
	unsigned FltRdyFlag;
	unsigned FltVldFlag;
	unsigned ProdRdyFlag;
	unsigned ProdVldFlag;

    // Data size parameters. These are set from the rotator's
    // parameters in the configure phase.
    unsigned m_nChannelCount;
    unsigned m_nBlockSize;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    unsigned m_nOverlapLength;
    audio_t m_fFFTScaler;

    void SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl);
    void SetCohMode(unsigned UseESP, unsigned CohPrtcl);

    void InitParams();
    void ConfigureAcc();
    void StartAcc();

    // Chain offload using regular accelerator invocation.
    // Here, pBFSrcDst is the shared BFormat passed between
    // tasks in the application.
    // m_Filters is a pointer to array containing the filters
    // for THIS CHANNEL ONLY.
    // m_pfScratchBufferA is the output buffer.
    // CurChannel is the current channel between operated on.
    void RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel, bool IsInit);

    // Chain offload using shared memory accelerator invocation.
    // Parameter definitions same as above.
    void NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel, bool IsInit);

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
    void BinaurOverlap(CBFormat* pBFSrcDst, audio_t* ppfDst, audio_t* m_pfOverlap, bool isLast);

    // Handle pipelined operation of binauralizer filter.
    void BinaurProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap);

    void PrintTimeInfo(unsigned factor, bool isPsycho = true);
};

#endif // FFICHAIN_H