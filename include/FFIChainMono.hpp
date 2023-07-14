#ifndef FFICHAINMONO_H
#define FFICHAINMONO_H

#if (USE_MONOLITHIC_ACC == 1)

void FFIChain::ConfigureAcc() {
	InitParams();

	// Allocate total memory, and assign the starting to a new pointer - 
	// sm_sync, that we use for managing synchronizations from CPU.
	mem = (device_t *) esp_alloc(mem_size);
    sm_sync = (volatile device_t*) mem;

	// Reset all sync variables to default values.
	for (unsigned memElem = 0; memElem < (mem_size/sizeof(device_t)); memElem++) {
		mem[memElem] = 0;
	}

	// Assign SpandexConfig and CoherenceMode based on compiler flags.
	SetSpandexConfig(IS_ESP, COH_MODE);
	SetCohMode(IS_ESP, COH_MODE);

	audio_ffi_cfg_000[0].logn_samples = logn_samples;

	audio_ffi_cfg_000[0].esp.coherence = CoherenceMode;

	audio_ffi_cfg_000[0].spandex_conf = SpandexConfig.spandex_reg;

    ffi_cfg_000[0].hw_buf = mem;

	InitSyncFlags();

	// Reset all sync variables to default values.
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		UpdateSync(ChainID*acc_len + VALID_FLAG_OFFSET, 0);
		UpdateSync(ChainID*acc_len + READY_FLAG_OFFSET, 1);
		UpdateSync(ChainID*acc_len + END_FLAG_OFFSET, 0);
	}

	UpdateSync(acc_len + FLT_VALID_FLAG_OFFSET, 0);
	UpdateSync(acc_len + FLT_READY_FLAG_OFFSET, 1);

    StartTime = 0;
    EndTime = 0;

	for (unsigned i = 0; i < N_TIME_MARKERS; i++)
    	TotalTime[i] = 0;
}

void FFIChain::StartAcc() {
    // Invoke accelerators but do not check for end
    audio_ffi_cfg_000[0].esp.start_stop = 1;
    
    esp_run(ffi_cfg_000, 1);
}

void FFIChain::InitSyncFlags() {
	audio_ffi_cfg_000[0].prod_valid_offset = VALID_FLAG_OFFSET;
	audio_ffi_cfg_000[0].prod_ready_offset = READY_FLAG_OFFSET;
	audio_ffi_cfg_000[0].flt_prod_valid_offset = acc_len + FLT_VALID_FLAG_OFFSET;
	audio_ffi_cfg_000[0].flt_prod_ready_offset = acc_len + FLT_READY_FLAG_OFFSET;
	audio_ffi_cfg_000[0].cons_valid_offset = (3 * acc_len) + VALID_FLAG_OFFSET;
	audio_ffi_cfg_000[0].cons_ready_offset = (3 * acc_len) + READY_FLAG_OFFSET;
	audio_ffi_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	audio_ffi_cfg_000[0].flt_input_offset = 5 * acc_len;
	audio_ffi_cfg_000[0].twd_input_offset = 7 * acc_len;
	audio_ffi_cfg_000[0].output_offset = (3 * acc_len) + SYNC_VAR_SIZE;
}

void FFIChain::PsychoRegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		StartCounter();
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		StartCounter();
		// Write input data for FIR filters.
		InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
		EndCounter(1);

		// Start and check for termination of each accelerator.
		StartCounter();
		esp_run(ffi_cfg_000, 1);
		EndCounter(2);

		// Read back output from IFFT
		StartCounter();
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		EndCounter(3);	

		ChannelsLeft--;
	}
}

void FFIChain::BinaurRegularProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			StartCounter();
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
			EndCounter(4);

			StartCounter();
			// Write input data for FIR filters.
			InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - ChannelsLeft]);
			EndCounter(5);

			// Start and check for termination of each accelerator.
			StartCounter();
    		esp_run(ffi_cfg_000, 1);
			EndCounter(6);

			// Read back output from IFFT
			StartCounter();
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			EndCounter(7);	

			ChannelsLeft--;
		}
	}
}

// Dummy functions
void FFIChain::FFTRegularProcess(kiss_fft_scalar* timedata, kiss_fft_scalar* freqdata) {}

void FFIChain::IFFTRegularProcess(kiss_fft_scalar* timedata, kiss_fft_scalar* freqdata) {}

#endif // USE_MONOLITHIC_ACC

#endif // FFICHAINMONO_H
