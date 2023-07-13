#ifndef DMAACC_H
#define DMAACC_H

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <AudioBase.hpp>

#define SLD_AUDIO_DMA 0x051
#define DMA_DEV_NAME "sld,audio_dma_stratus"

#define AUDIO_DMA_START_OFFSET_REG 0x40

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
#define LOAD_STORE_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4
#define FLT_VALID_FLAG_OFFSET 6
#define FLT_READY_FLAG_OFFSET 8

#define NUM_CFG_REG 8

#define RD_SIZE SYNC_VAR_SIZE
#define RD_SP_OFFSET SYNC_VAR_SIZE + 1
#define MEM_SRC_OFFSET SYNC_VAR_SIZE + 2
#define WR_SIZE SYNC_VAR_SIZE + 3
#define WR_SP_OFFSET SYNC_VAR_SIZE + 4
#define MEM_DST_OFFSET SYNC_VAR_SIZE + 5
#define CONS_VALID_OFFSET SYNC_VAR_SIZE + 6
#define CONS_READY_OFFSET SYNC_VAR_SIZE + 7

class DMAAcc : public AudioBase {
public:
    /* <<--Device pointers-->> */
    int nDev;
    struct esp_device *espdevs;
    struct esp_device *DMADev;

    /* <<--Device params-->> */
    device_t *mem;
    unsigned **ptable;
    volatile device_t* sm_sync;

    // Accelerator parameters.
    unsigned mem_size;
    unsigned acc_len;
    unsigned acc_size;
    unsigned dma_offset;

    // Data size parameters.
    unsigned m_nChannelCount;
    unsigned m_nBlockSize;
    unsigned m_nFFTBins;

    // Sync flag offsets.
	unsigned ConsRdyFlag;
	unsigned ConsVldFlag;
	unsigned FltRdyFlag;
	unsigned FltVldFlag;
	unsigned ProdRdyFlag;
	unsigned ProdVldFlag;

    // Scratchpad offsets
    unsigned init_data_offset;
    unsigned psycho_filters_offset;

    unsigned SpandexReg;
    unsigned CoherenceMode;

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void ConfigureSPXRegister();

    void StartAcc();

    void TerminateAcc();

    void LoadAllData();

    void StoreInputData(unsigned InitChannel);

    void StorePsychoFilters(unsigned InitChannel);
    
    void StoreBinaurFilters(unsigned InitEar, unsigned InitChannel);
};

#endif // DMAACC_H