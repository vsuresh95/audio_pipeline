#ifndef ROTATEACC_H
#define ROTATEACC_H

/* User defined registers */
/* <<--regs-->> */
#define HU_AUDIODEC_CFG_REGS_31_REG 0xbc
#define HU_AUDIODEC_CFG_REGS_30_REG 0xb8
#define HU_AUDIODEC_CFG_REGS_26_REG 0xb4
#define HU_AUDIODEC_CFG_REGS_27_REG 0xb0
#define HU_AUDIODEC_CFG_REGS_24_REG 0xac
#define HU_AUDIODEC_CFG_REGS_25_REG 0xa8
#define HU_AUDIODEC_CFG_REGS_22_REG 0xa4
#define HU_AUDIODEC_CFG_REGS_23_REG 0xa0
#define HU_AUDIODEC_CFG_REGS_8_REG 0x9c
#define HU_AUDIODEC_CFG_REGS_20_REG 0x98
#define HU_AUDIODEC_CFG_REGS_9_REG 0x94
#define HU_AUDIODEC_CFG_REGS_21_REG 0x90
#define HU_AUDIODEC_CFG_REGS_6_REG 0x8c
#define HU_AUDIODEC_CFG_REGS_7_REG 0x88
#define HU_AUDIODEC_CFG_REGS_4_REG 0x84
#define HU_AUDIODEC_CFG_REGS_5_REG 0x80
#define HU_AUDIODEC_CFG_REGS_2_REG 0x7c
#define HU_AUDIODEC_CFG_REGS_3_REG 0x78
#define HU_AUDIODEC_CFG_REGS_0_REG 0x74
#define HU_AUDIODEC_CFG_REGS_28_REG 0x70
#define HU_AUDIODEC_CFG_REGS_1_REG 0x6c
#define HU_AUDIODEC_CFG_REGS_29_REG 0x68
#define HU_AUDIODEC_CFG_REGS_19_REG 0x64
#define HU_AUDIODEC_CFG_REGS_18_REG 0x60
#define HU_AUDIODEC_CFG_REGS_17_REG 0x5c
#define HU_AUDIODEC_CFG_REGS_16_REG 0x58
#define HU_AUDIODEC_CFG_REGS_15_REG 0x54
#define HU_AUDIODEC_CFG_REGS_14_REG 0x50
#define HU_AUDIODEC_CFG_REGS_13_REG 0x4c
#define HU_AUDIODEC_CFG_REGS_12_REG 0x48
#define HU_AUDIODEC_CFG_REGS_11_REG 0x44
#define HU_AUDIODEC_CFG_REGS_10_REG 0x40

#define SLD_HU_AUDIODEC 0x077
#define ROTATE_DEV_NAME "sld,hu_audiodec_rtl"

class RotateAcc {
public:
    /* <<--Device pointers-->> */
    int nDev;
    struct esp_device *espdevs;
    struct esp_device *RotateDev;

    /* <<--Device params-->> */
    unsigned **ptable;

    unsigned logn_samples;

    unsigned mem_size;
    unsigned acc_size;
    unsigned m_nChannelCount;

    unsigned SpandexReg;
    unsigned CoherenceMode;

    unsigned cfg_block_size;
    unsigned cfg_input_base;
    unsigned cfg_output_base;

	unsigned m_fCosAlpha;
	unsigned m_fSinAlpha;
	unsigned m_fCosBeta;
	unsigned m_fSinBeta;
	unsigned m_fCosGamma;
	unsigned m_fSinGamma;
	unsigned m_fCos2Alpha;
	unsigned m_fSin2Alpha;
	unsigned m_fCos2Beta;
	unsigned m_fSin2Beta;
	unsigned m_fCos2Gamma;
	unsigned m_fSin2Gamma;
	unsigned m_fCos3Alpha;
	unsigned m_fSin3Alpha;
	unsigned m_fCos3Beta;
	unsigned m_fSin3Beta;
	unsigned m_fCos3Gamma;
	unsigned m_fSin3Gamma;

    void ProbeAcc(int DeviceNum);

    void ConfigureAcc();

    void StartAcc();

    void TerminateAcc();
};

#endif // ROTATEACC_H