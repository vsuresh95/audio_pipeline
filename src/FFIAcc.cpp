#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FFIAcc.hpp>

void FFIAcc::ProbeAcc(int DeviceNum) {
    nDev = probe(&espdevs, VENDOR_SLD, SLD_AUDIO_FFI, FFI_DEV_NAME);
	if (nDev == 0) {
		printf("%s not found\n", FFI_DEV_NAME);
		return;
	}

	FFIDev = &espdevs[DeviceNum];
}

void FFIAcc::ConfigureAcc() {
	iowrite32(FFIDev, SPANDEX_REG, SpandexReg);

	// Pass common configuration parameters
	iowrite32(FFIDev, SELECT_REG, ioread32(FFIDev, DEVID_REG));
	iowrite32(FFIDev, COHERENCE_REG, CoherenceMode);

	iowrite32(FFIDev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(FFIDev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(FFIDev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(FFIDev, SRC_OFFSET_REG, 0);
	iowrite32(FFIDev, DST_OFFSET_REG, 0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(FFIDev, AUDIO_FFI_LOGN_SAMPLES_REG, logn_samples);
}

void FFIAcc::StartAcc() {
	// Start accelerators
	iowrite32(FFIDev, CMD_REG, CMD_MASK_START);
}

void FFIAcc::TerminateAcc() {
    unsigned done;

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(FFIDev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}

	iowrite32(FFIDev, CMD_REG, 0x0);
}