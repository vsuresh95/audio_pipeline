#ifndef FIRACC_H
#define FIRACC_H

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FIR_DO_INVERSE_REG 0x48
#define AUDIO_FIR_LOGN_SAMPLES_REG 0x44
#define AUDIO_FIR_DO_SHIFT_REG 0x40

#define SLD_AUDIO_FIR 0x055
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

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void StartAcc();

    void TerminateAcc();
};

#endif // FIRACC_H