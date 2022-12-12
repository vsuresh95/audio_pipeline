#ifndef FFTACC_H
#define FFTACC_H

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFT_DO_SHIFT_REG 0x40

#define SLD_AUDIO_FFT 0x056
#define FFT_DEV_NAME "sld,audio_fft_stratus"

class FFTAcc {
public:
    /* <<--Device pointers-->> */
    int nDev;
    struct esp_device *espdevs;
    struct esp_device *FFTDev;

    /* <<--Device params-->> */
    unsigned **ptable;

    unsigned inverse;
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

#endif // FFTACC_H