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

void FFIChain::RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel, bool IsInit) {
	// Write input data for FFT.
    StartCounter();
	InitData(pBFSrcDst, CurChannel, IsInit);
    EndCounter(0);

	// Write input data for FIR filters.
    StartCounter();
	InitFilters(pBFSrcDst, m_Filters);
    EndCounter(1);

	// Start and check for termination of each accelerator.
    StartCounter();
    esp_run(ffi_cfg_000, 1);
    EndCounter(2);

	// Read back output from IFFT.
    StartCounter();
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
    EndCounter(3);
}

void FFIChain::NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel, bool IsInit) {
    StartCounter();
	// Wait for FFT (consumer) to be ready.
	while (sm_sync[ConsRdyFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[ConsRdyFlag] = 0;
	// Write input data for FFT.
	InitData(pBFSrcDst, CurChannel, IsInit);
    EndCounter(0);

    StartCounter();
	// Wait for FIR (consumer) to be ready.
	while (sm_sync[FltRdyFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[FltRdyFlag] = 0;
	// Write input data for FIR filters.
	InitFilters(pBFSrcDst, m_Filters);
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
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
	// Inform IFFT (producer) - ready for next iteration.
	sm_sync[ProdRdyFlag] = 1;
    EndCounter(3);
}
#endif // USE_MONOLITHIC_ACC

#endif // FFICHAINMONO_H
