#ifndef FFIACC_H
#define FFIACC_H

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FFI_DO_INVERSE_REG 0x48
#define AUDIO_FFI_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFI_DO_SHIFT_REG 0x40

#define AUDIO_FFI_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFI_PROD_READY_OFFSET 0x50
#define AUDIO_FFI_FLT_PROD_VALID_OFFSET 0x54
#define AUDIO_FFI_FLT_PROD_READY_OFFSET 0x58
#define AUDIO_FFI_CONS_VALID_OFFSET 0x5C
#define AUDIO_FFI_CONS_READY_OFFSET 0x60
#define AUDIO_FFI_INPUT_OFFSET 0x64
#define AUDIO_FFI_FLT_INPUT_OFFSET 0x68
#define AUDIO_FFI_TWD_INPUT_OFFSET 0x6C
#define AUDIO_FFI_OUTPUT_OFFSET 0x70

#define SLD_AUDIO_FFI 0x057
#define FFI_DEV_NAME "sld,audio_ffi_stratus"

class FFIAcc {
public:
    /* <<--Device pointers-->> */
    int nDev;
    struct esp_device *espdevs;
    struct esp_device *FFIDev;

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

#endif // FFIACC_H