#ifndef FFICHAINMONO_H
#define FFICHAINMONO_H

#ifdef USE_MONOLITHIC_ACC

void FFIChain::ConfigureAcc() {
    Name = (char *) "FFI CHAIN";
	InitParams();

	// Allocate total memory, and assign the starting to a new pointer - 
	// sm_sync, that we use for managing synchronizations from CPU.
	mem = (device_t *) aligned_malloc(mem_size);
    sm_sync = (volatile device_t*) mem;

	// Allocate and populate page table. All accelerators share the same page tables.
	ptable = (unsigned **) aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (unsigned i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(device_t))];

	// Assign SpandexConfig and CoherenceMode based on compiler flags.
	SetSpandexConfig(IS_ESP, COH_MODE);
	SetCohMode(IS_ESP, COH_MODE);

	// Find FFT, FIR and IFFT devices using ESP's probe function.
	// The parameters might need to be modified if you have more than 2 FFTs
	// or 1 FIR in your system, that you want to map differently.
	FFIInst.ProbeAcc(0);

	// Assign parameters to all accelerator objects.
	FFIInst.logn_samples = logn_samples;
	FFIInst.ptable = ptable;
    FFIInst.mem_size = mem_size;
    FFIInst.acc_size = acc_size;
    FFIInst.SpandexReg = SpandexConfig.spandex_reg;
    FFIInst.CoherenceMode =  CoherenceMode;

	// After assigning the parameters, we call ConfigureAcc() for
	// each accelerator object to program accelerator register
	// with these parameters.
	FFIInst.ConfigureAcc();

	// Reset all sync variables to default values.
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		sm_sync[ChainID*acc_len + VALID_FLAG_OFFSET] = 0;
		sm_sync[ChainID*acc_len + READY_FLAG_OFFSET] = 1;
		sm_sync[ChainID*acc_len + END_FLAG_OFFSET] = 0;
	}

	sm_sync[acc_len + FLT_VALID_FLAG_OFFSET] = 0;
	sm_sync[acc_len + FLT_READY_FLAG_OFFSET] = 1;
}

void FFIChain::StartAcc() {
	// We have separeted ConfigureAcc() from StartAcc() in case we
	// don't use shared memory chaining.
	FFIInst.StartAcc();
}

void FFIChain::RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel, bool IsInit) {
	// Write input data for FFT.
	InitData(pBFSrcDst, CurChannel, IsInit);
	// Write input data for FIR filters.
	InitFilters(pBFSrcDst, m_Filters);

	// Start and check for termination of each accelerator.
	FFIInst.StartAcc();
	FFIInst.TerminateAcc();

	// Read back output from IFFT.
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
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

	// Wait for IFFT (producer) to send output.
	while (sm_sync[ProdVldFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[ProdVldFlag] = 0;
	// Read back output from IFFT
    StartCounter();
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
    EndCounter(2);
	// Inform IFFT (producer) - ready for next iteration.
	sm_sync[ProdRdyFlag] = 1;
}
#endif // USE_MONOLITHIC_ACC

#endif // FFICHAINMONO_H
