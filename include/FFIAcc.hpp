#ifndef FFIACC_H
#define FFIACC_H

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FFI_DO_INVERSE_REG 0x48
#define AUDIO_FFI_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFI_DO_SHIFT_REG 0x40

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

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void StartAcc();

    void TerminateAcc();
};

#endif // FFIACC_H