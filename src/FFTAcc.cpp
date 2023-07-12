#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <FFTAcc.hpp>

void FFTAcc::ProbeAcc(int DeviceNum) {
    nDev = probe(&espdevs, VENDOR_SLD, SLD_AUDIO_FFT, FFT_DEV_NAME);
	if (nDev == 0) {
		printf("%s not found\n", FFT_DEV_NAME);
		return;
	}

	FFTDev = &espdevs[DeviceNum];
}

void FFTAcc::ConfigureAcc() {
	iowrite32(FFTDev, SPANDEX_REG, SpandexReg);

	// Pass common configuration parameters
	iowrite32(FFTDev, SELECT_REG, ioread32(FFTDev, DEVID_REG));
	iowrite32(FFTDev, COHERENCE_REG, CoherenceMode);

	iowrite32(FFTDev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(FFTDev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(FFTDev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(FFTDev, SRC_OFFSET_REG, 2 * inverse * acc_size);
	iowrite32(FFTDev, DST_OFFSET_REG, 2 * inverse * acc_size);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(FFTDev, AUDIO_FFT_LOGN_SAMPLES_REG, logn_samples);
	iowrite32(FFTDev, AUDIO_FFT_DO_INVERSE_REG, inverse);

	iowrite32(FFTDev, AUDIO_FFT_PROD_VALID_OFFSET, prod_valid_offset);
	iowrite32(FFTDev, AUDIO_FFT_PROD_READY_OFFSET, prod_ready_offset);
	iowrite32(FFTDev, AUDIO_FFT_CONS_VALID_OFFSET, cons_valid_offset);
	iowrite32(FFTDev, AUDIO_FFT_CONS_READY_OFFSET, cons_ready_offset);
	iowrite32(FFTDev, AUDIO_FFT_LOAD_DATA_OFFSET, load_data_offset);
	iowrite32(FFTDev, AUDIO_FFT_STORE_DATA_OFFSET, store_data_offset);
}

void FFTAcc::StartAcc() {
	// Start accelerators
	iowrite32(FFTDev, CMD_REG, CMD_MASK_START);
}

void FFTAcc::TerminateAcc() {
    unsigned done;

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(FFTDev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}

	iowrite32(FFTDev, CMD_REG, 0x0);
}