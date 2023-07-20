#ifndef FFI_CHAIN_HELPER
#define FFI_CHAIN_HELPER

#include <ReadWriteCodeHelper.hpp>

void FFIChain::InitParams() {
	// Number of samples for the FFT or FIR accelerator. (1024)
    num_samples = 1 << logn_samples;

	// Total number of samples will be 2x due to complex number pairs.
	// Size is calculated for the data type that the device works on.
	// Here, device_t can be int (4B) or fixed-point (4B).
	in_len = 2 * num_samples;
	out_len = 2 * num_samples;
	in_size = in_len * sizeof(device_t);
	out_size = out_len * sizeof(device_t);

	// We add the size of control partition to determine total number
	// of elements for each accelerator.
    acc_len = SYNC_VAR_SIZE + out_len;
    acc_size = acc_len * sizeof(device_t);

	// Total size of data for each accelerator
	mem_size = acc_size;

	// Total size for all accelerators combined:
	// 0*acc_size - 1*acc_size: FFT Input
	// 1*acc_size - 2*acc_size: FIR Input
	// 2*acc_size - 3*acc_size: IFFT Input
	// 3*acc_size - 4*acc_size: CPU Input
	// 4*acc_size - 5*acc_size: Unused
	// 5*acc_size - 7*acc_size: FIR filters
	// 7*acc_size - 7*acc_size: Twiddle factors
	// 8*acc_size - 9*acc_size: DMA input
	// Therefore, NUM_DEVICES+7 = 10.
    mem_size = acc_size * (NUM_DEVICES+7);

	// Extra space to store rotate order accelerator data
    mem_size += 2 * num_samples * m_nChannelCount;

	// Helper flags for sync flags that the CPU needs to access.
	// We add a pair of sync flags for FIR filter weights, so that
	// this write can be overlapped with FFT operations.
	ConsRdyFlag = 0*acc_len + READY_FLAG_OFFSET;
	ConsVldFlag = 0*acc_len + VALID_FLAG_OFFSET;
	FltRdyFlag = 1*acc_len + FLT_READY_FLAG_OFFSET;
	FltVldFlag = 1*acc_len + FLT_VALID_FLAG_OFFSET;
	ProdRdyFlag = 3*acc_len + READY_FLAG_OFFSET;
	ProdVldFlag = 3*acc_len + VALID_FLAG_OFFSET;
	DMARdyFlag = 8*acc_len + READY_FLAG_OFFSET;
	DMAVldFlag = 8*acc_len + VALID_FLAG_OFFSET;
}

#if (USE_MONOLITHIC_ACC == 0)
void FFIChain::InitSyncFlags() {
	audio_fft_cfg_000[0].prod_valid_offset = VALID_FLAG_OFFSET;
	audio_fft_cfg_000[0].prod_ready_offset = READY_FLAG_OFFSET;
	audio_fft_cfg_000[0].cons_valid_offset = acc_len + VALID_FLAG_OFFSET;
	audio_fft_cfg_000[0].cons_ready_offset = acc_len + READY_FLAG_OFFSET;
	audio_fft_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	audio_fft_cfg_000[0].output_offset = acc_len + SYNC_VAR_SIZE;

	audio_fir_cfg_000[0].prod_valid_offset = acc_len + VALID_FLAG_OFFSET;
	audio_fir_cfg_000[0].prod_ready_offset = acc_len + READY_FLAG_OFFSET;
	audio_fir_cfg_000[0].flt_prod_valid_offset = acc_len + FLT_VALID_FLAG_OFFSET;
	audio_fir_cfg_000[0].flt_prod_ready_offset = acc_len + FLT_READY_FLAG_OFFSET;
	audio_fir_cfg_000[0].cons_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	audio_fir_cfg_000[0].cons_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	audio_fir_cfg_000[0].input_offset = acc_len + SYNC_VAR_SIZE;
	audio_fir_cfg_000[0].flt_input_offset = 5 * acc_len;
	audio_fir_cfg_000[0].twd_input_offset = 7 * acc_len;
	audio_fir_cfg_000[0].output_offset = (2 * acc_len) + SYNC_VAR_SIZE;

	audio_ifft_cfg_000[0].prod_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	audio_ifft_cfg_000[0].prod_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	audio_ifft_cfg_000[0].cons_valid_offset = (3 * acc_len) + VALID_FLAG_OFFSET;
	audio_ifft_cfg_000[0].cons_ready_offset = (3 * acc_len) + READY_FLAG_OFFSET;
	audio_ifft_cfg_000[0].input_offset = (2 * acc_len) + SYNC_VAR_SIZE;
	audio_ifft_cfg_000[0].output_offset = (3 * acc_len) + SYNC_VAR_SIZE;
}

void FFIChain::StartAcc() {
    // Invoke accelerators but do not check for end
    audio_fft_cfg_000[0].esp.start_stop = 1;
    audio_fir_cfg_000[0].esp.start_stop = 1;
    audio_ifft_cfg_000[0].esp.start_stop = 1;
    
    esp_run(fft_cfg_000, 1);
    esp_run(fir_cfg_000, 1);
    esp_run(ifft_cfg_000, 1);
}
#endif

void FFIChain::EndAcc() {
	// Set all END variables to 1.
	for (unsigned ChainID = 0; ChainID < 3; ChainID++) {
		UpdateSync(ChainID*acc_len + END_FLAG_OFFSET, 1);
	}

	// Wait for FFT (consumer) to be ready.
	SpinSync(ConsRdyFlag, 1);
	// Reset flag for next iteration.
	UpdateSync(ConsRdyFlag, 0);
	// Inform FFT (consumer) to start.
	UpdateSync(ConsVldFlag, 1);

	// Wait for FIR (consumer) to be ready.
	SpinSync(FltRdyFlag, 1);
	// Reset flag for next iteration.
	UpdateSync(FltRdyFlag, 0);
	// Inform FIR (consumer) to start.
	UpdateSync(FltVldFlag, 1);

	// Wait for IFFT (producer) to send output.
	SpinSync(ProdVldFlag, 1);
	// Reset flag for next iteration.
	UpdateSync(ProdVldFlag, 0);
	// Inform IFFT (producer) - ready for next iteration.
	UpdateSync(ProdRdyFlag, 1);
}

void FFIChain::SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl) {
	if (UseESP) {
		SpandexConfig.spandex_reg = 0;
	} else {
		if (CohPrtcl == SPX_OWNER_PREDICTION) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 2;
			SpandexConfig.w_en = 1;
			SpandexConfig.w_op = 1;
			SpandexConfig.w_type = 1;
		} else if (CohPrtcl == SPX_WRITE_THROUGH_FWD) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 2;
			SpandexConfig.w_en = 1;
			SpandexConfig.w_type = 1;
		} else if (CohPrtcl == SPX_BASELINE_REQV) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 1;
		} else {
			SpandexConfig.spandex_reg = 0;
		}
	}
}

void FFIChain::SetCohMode(unsigned UseESP, unsigned CohPrtcl) {
	if (!UseESP) {
		CoherenceMode = ACC_COH_FULL;
	} else {
		if (CohPrtcl == ESP_NON_COHERENT_DMA) {
			CoherenceMode = ACC_COH_NONE;
		} else if (CohPrtcl == ESP_LLC_COHERENT_DMA) {
			CoherenceMode = ACC_COH_LLC;
		} else if (CohPrtcl == ESP_COHERENT_DMA) {
			CoherenceMode = ACC_COH_RECALL;
		} else {
			CoherenceMode = ACC_COH_FULL;
		}
	}    
}

#endif // FFI_CHAIN_HELPER