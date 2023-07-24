#ifndef FFTACC_H
#define FFTACC_H

/* User defined registers */
/* <<--regs-->> */
#if (EPOCHS_TARGET == 0)
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFT_DO_SHIFT_REG 0x40
#else
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x40
#define AUDIO_FFT_NUM_FFTS_REG 0x44
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_DO_SHIFT_REG 0x4c
#define AUDIO_FFT_SCALE_FACTOR_REG 0x50
#endif

#define AUDIO_FFT_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFT_PROD_READY_OFFSET 0x50
#define AUDIO_FFT_CONS_VALID_OFFSET 0x54
#define AUDIO_FFT_CONS_READY_OFFSET 0x58
#define AUDIO_FFT_INPUT_OFFSET 0x5C
#define AUDIO_FFT_OUTPUT_OFFSET 0x60

#if (EPOCHS_TARGET == 0)
#define SLD_AUDIO_FFT 0x055
#define FFT_DEV_NAME "sld,audio_fft_stratus"
#else
#define SLD_AUDIO_FFT 0x057
#define FFT_DEV_NAME "sld,fft2_stratus"
#endif

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
    unsigned sync_size;

    unsigned SpandexReg;
    unsigned CoherenceMode;

	// ASI sync flag offsets
    unsigned prod_valid_offset;
    unsigned prod_ready_offset;
    unsigned cons_valid_offset;
    unsigned cons_ready_offset;
    unsigned input_offset;
    unsigned output_offset;

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void StartAcc();

    void TerminateAcc();
};

#endif // FFTACC_H