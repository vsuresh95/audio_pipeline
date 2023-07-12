#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FIRAcc.hpp>

void FIRAcc::ProbeAcc(int DeviceNum) {
    nDev = probe(&espdevs, VENDOR_SLD, SLD_AUDIO_FIR, FIR_DEV_NAME);
	if (nDev == 0) {
		printf("%s not found\n", FIR_DEV_NAME);
		return;
	}

	FIRDev = &espdevs[DeviceNum];
}

void FIRAcc::ConfigureAcc() {
	iowrite32(FIRDev, SPANDEX_REG, SpandexReg);

	// Pass common configuration parameters
	iowrite32(FIRDev, SELECT_REG, ioread32(FIRDev, DEVID_REG));
	iowrite32(FIRDev, COHERENCE_REG, CoherenceMode);

	iowrite32(FIRDev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(FIRDev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(FIRDev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(FIRDev, SRC_OFFSET_REG, acc_size);
	iowrite32(FIRDev, DST_OFFSET_REG, acc_size);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(FIRDev, AUDIO_FIR_LOGN_SAMPLES_REG, logn_samples);

	iowrite32(FIRDev, AUDIO_FIR_PROD_VALID_OFFSET, prod_valid_offset);
	iowrite32(FIRDev, AUDIO_FIR_PROD_READY_OFFSET, prod_ready_offset);
	iowrite32(FIRDev, AUDIO_FIR_FLT_PROD_VALID_OFFSET, flt_prod_valid_offset);
	iowrite32(FIRDev, AUDIO_FIR_FLT_PROD_READY_OFFSET, flt_prod_ready_offset);
	iowrite32(FIRDev, AUDIO_FIR_CONS_VALID_OFFSET, cons_valid_offset);
	iowrite32(FIRDev, AUDIO_FIR_CONS_READY_OFFSET, cons_ready_offset);
	iowrite32(FIRDev, AUDIO_FIR_LOAD_DATA_OFFSET, load_data_offset);
	iowrite32(FIRDev, AUDIO_FIR_FLT_LOAD_DATA_OFFSET, flt_load_data_offset);
	iowrite32(FIRDev, AUDIO_FIR_TWD_LOAD_DATA_OFFSET, twd_load_data_offset);
	iowrite32(FIRDev, AUDIO_FIR_STORE_DATA_OFFSET, store_data_offset);
}

void FIRAcc::StartAcc() {
	// Start accelerators
	iowrite32(FIRDev, CMD_REG, CMD_MASK_START);
}

void FIRAcc::TerminateAcc() {
    unsigned done;

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(FIRDev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}

	iowrite32(FIRDev, CMD_REG, 0x0);
}