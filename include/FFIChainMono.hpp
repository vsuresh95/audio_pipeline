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

	// Reset all sync variables to default values.
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		sm_sync[ChainID*acc_len + VALID_FLAG_OFFSET] = 0;
		sm_sync[ChainID*acc_len + READY_FLAG_OFFSET] = 1;
		sm_sync[ChainID*acc_len + END_FLAG_OFFSET] = 0;
	}

	sm_sync[acc_len + FLT_VALID_FLAG_OFFSET] = 0;
	sm_sync[acc_len + FLT_READY_FLAG_OFFSET] = 1;

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

void FFIChain::PsychoNonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		StartCounter();
		// Wait for FFT (consumer) to be ready.
		while (sm_sync[ConsRdyFlag] != 1);
		// Reset flag for next iteration.
		sm_sync[ConsRdyFlag] = 0;
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		StartCounter();
		// Wait for FIR (consumer) to be ready.
		while (sm_sync[FltRdyFlag] != 1);
		// Reset flag for next iteration.
		sm_sync[FltRdyFlag] = 0;
		// Write input data for FIR filters.
		InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
		// Inform FIR (consumer) of filters ready.
		sm_sync[FltVldFlag] = 1;
		// Inform FFT (consumer) to start.
		sm_sync[ConsVldFlag] = 1;
		EndCounter(1);

		StartCounter();
		// Wait for IFFT (producer) to send output.
		while (sm_sync[ProdVldFlag] != 1);
		// Reset flag for next iteration.
		sm_sync[ProdVldFlag] = 0;
		EndCounter(2);

		// Read back output from IFFT
		StartCounter();
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		// Inform IFFT (producer) - ready for next iteration.
		sm_sync[ProdRdyFlag] = 1;
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

void FFIChain::BinaurNonPipelineProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			StartCounter();
			// Wait for FFT (consumer) to be ready.
			while (sm_sync[ConsRdyFlag] != 1);
			// Reset flag for next iteration.
			sm_sync[ConsRdyFlag] = 0;
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
			EndCounter(4);

			StartCounter();
			// Wait for FIR (consumer) to be ready.
			while (sm_sync[FltRdyFlag] != 1);
			// Reset flag for next iteration.
			sm_sync[FltRdyFlag] = 0;
			// Write input data for FIR filters.
			InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - ChannelsLeft]);
			// Inform FIR (consumer) of filters ready.
			sm_sync[FltVldFlag] = 1;
			// Inform FFT (consumer) to start.
			sm_sync[ConsVldFlag] = 1;
			EndCounter(5);			

			StartCounter();
			// Wait for IFFT (producer) to send output.
			while (sm_sync[ProdVldFlag] != 1);
			// Reset flag for next iteration.
			sm_sync[ProdVldFlag] = 0;
			EndCounter(6);

			// Read back output from IFFT
			StartCounter();
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			// Inform IFFT (producer) - ready for next iteration.
			sm_sync[ProdRdyFlag] = 1;
			EndCounter(7);	

			ChannelsLeft--;
		}
	}
}

#endif // USE_MONOLITHIC_ACC

#endif // FFICHAINMONO_H
