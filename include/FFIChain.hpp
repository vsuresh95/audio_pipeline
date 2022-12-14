#ifndef FFICHAIN_H
#define FFICHAIN_H

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FFTAcc.hpp>
#include <FIRAcc.hpp>

#include <kiss_fftr.hpp>
#include <BFormat.hpp>

#if (USE_INT == 1)
#define FLOAT_TO_FIXED_WRAP(x, y) x
#define FIXED_TO_FLOAT_WRAP(x, y) x
#else
#include <fixed_point.h>
#define AUDIO_FX_IL 4
#define FLOAT_TO_FIXED_WRAP(x, y) float_to_fixed32(x, y)
#define FIXED_TO_FLOAT_WRAP(x, y) fixed32_to_float(x, y)
#endif

// Size and parameter defines
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
    FFTAcc FFTInst;
    FIRAcc FIRInst;
    FFTAcc IFFTInst;

    unsigned logn_samples;
    unsigned num_samples;

    device_t*mem;
    unsigned **ptable;

    unsigned in_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned out_offset;
    unsigned mem_size;
    unsigned acc_offset;
    unsigned acc_size;

	unsigned ConsRdyFlag;
	unsigned ConsVldFlag;
	unsigned FltRdyFlag;
	unsigned FltVldFlag;
	unsigned ProdRdyFlag;
	unsigned ProdVldFlag;
     
    volatile device_t* sm_sync;

    spandex_config_t SpandexConfig;
    unsigned CoherenceMode;

    void SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl);
    void SetCohMode(unsigned UseESP, unsigned CohPrtcl);

    void InitParams();
    void ConfigureAcc();
    void StartAcc();

    void RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel);
    void NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel);
    // void PipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters);

    void InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles);
    void InitData(CBFormat* pBFSrcDst, unsigned niChannel);
    void InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters);
    void ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA, unsigned niChannel);
};

#endif // FFICHAIN_H