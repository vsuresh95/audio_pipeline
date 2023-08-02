#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <RotateAcc.hpp>

void RotateAcc::ProbeAcc(int DeviceNum) {
    nDev = probe(&espdevs, VENDOR_SLD, SLD_HU_AUDIODEC, ROTATE_DEV_NAME);
	if (nDev == 0) {
		printf("%s not found\n", ROTATE_DEV_NAME);
		return;
	}

	RotateDev = &espdevs[DeviceNum];
}

void RotateAcc::ConfigureAcc() {
	iowrite32(RotateDev, SPANDEX_REG, SpandexReg);

	// Pass common configuration parameters
	iowrite32(RotateDev, SELECT_REG, ioread32(RotateDev, DEVID_REG));
	iowrite32(RotateDev, COHERENCE_REG, CoherenceMode);

	iowrite32(RotateDev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(RotateDev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(RotateDev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(RotateDev, SRC_OFFSET_REG, 10 * acc_size);
	iowrite32(RotateDev, DST_OFFSET_REG, (10 * acc_size) + (m_nChannelCount * BLOCK_SIZE * sizeof(device_t)));

	cfg_block_size = BLOCK_SIZE;
	cfg_input_base = 0;
	cfg_output_base = 0;

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_2_REG, cfg_block_size);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_3_REG, cfg_input_base);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_4_REG, cfg_output_base);
}

void RotateAcc::StartAcc() {
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_8_REG, m_fCosAlpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_9_REG, m_fSinAlpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_10_REG, m_fCosBeta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_11_REG, m_fSinBeta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_12_REG, m_fCosGamma);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_13_REG, m_fSinGamma);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_14_REG, m_fCos2Alpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_15_REG, m_fSin2Alpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_16_REG, m_fCos2Beta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_17_REG, m_fSin2Beta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_18_REG, m_fCos2Gamma);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_19_REG, m_fSin2Gamma);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_20_REG, m_fCos3Alpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_21_REG, m_fSin3Alpha);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_22_REG, m_fCos3Beta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_23_REG, m_fSin3Beta);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_24_REG, m_fCos3Gamma);
	iowrite32(RotateDev, HU_AUDIODEC_CFG_REGS_25_REG, m_fSin3Gamma);

	// Start accelerators
	iowrite32(RotateDev, CMD_REG, CMD_MASK_START);
}

void RotateAcc::TerminateAcc() {
    unsigned done;

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(RotateDev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}

	iowrite32(RotateDev, CMD_REG, 0x0);
}