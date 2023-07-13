#ifndef FIRACC_H
#define FIRACC_H

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FIR_DO_INVERSE_REG 0x48
#define AUDIO_FIR_LOGN_SAMPLES_REG 0x44
#define AUDIO_FIR_DO_SHIFT_REG 0x40

#define AUDIO_FIR_PROD_VALID_OFFSET 0x4C
#define AUDIO_FIR_PROD_READY_OFFSET 0x50
#define AUDIO_FIR_FLT_PROD_VALID_OFFSET 0x54
#define AUDIO_FIR_FLT_PROD_READY_OFFSET 0x58
#define AUDIO_FIR_CONS_VALID_OFFSET 0x5C
#define AUDIO_FIR_CONS_READY_OFFSET 0x60
#define AUDIO_FIR_INPUT_OFFSET 0x64
#define AUDIO_FIR_FLT_INPUT_OFFSET 0x68
#define AUDIO_FIR_TWD_INPUT_OFFSET 0x6C
#define AUDIO_FIR_OUTPUT_OFFSET 0x70

#define SLD_AUDIO_FIR 0x056
#define FIR_DEV_NAME "sld,audio_fir_stratus"

class FIRAcc {
public:
    /* <<--Device pointers-->> */
    int nDev;
    struct esp_device *espdevs;
    struct esp_device *FIRDev;

    /* <<--Device params-->> */
    unsigned **ptable;

    unsigned logn_samples;

    unsigned mem_size;
    unsigned acc_size;

    unsigned SpandexReg;
    unsigned CoherenceMode;

	// ASI sync flag offsets
    unsigned prod_valid_offset;
    unsigned prod_ready_offset;
    unsigned flt_prod_valid_offset;
    unsigned flt_prod_ready_offset;
    unsigned cons_valid_offset;
    unsigned cons_ready_offset;
    unsigned input_offset;
    unsigned flt_input_offset;
    unsigned twd_input_offset;
    unsigned output_offset;

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void StartAcc();

    void TerminateAcc();
};

#endif // FIRACC_H