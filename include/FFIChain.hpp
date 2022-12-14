#ifndef FFICHAIN_H
#define FFICHAIN_H

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FFTAcc.hpp>
#include <FIRAcc.hpp>

#include <kiss_fftr.hpp>
#include <BFormat.hpp>

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

class FFIChain {
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

    void SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl);
    void SetCohMode(unsigned UseESP, unsigned CohPrtcl);

    void InitParams();
    void ConfigureAcc();
    void StartAcc();

    void RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel);
    void NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel);

    void InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles);
    void InitData(CBFormat* pBFSrcDst, unsigned niChannel);
    void InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters);
    void ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA, unsigned niChannel);

    // void PipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters);
};

#endif // FFICHAIN_H