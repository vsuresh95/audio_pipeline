#ifndef FFICHAINMONO_H
#define FFICHAINMONO_H

#if (USE_MONOLITHIC_ACC == 1)

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

void FFIChain::PsychoRegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		WriteScratchReg(0x2);
		StartCounter();
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);
		WriteScratchReg(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		WriteScratchReg(0x4);
		StartCounter();
		// Write input data for FIR filters.
		InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
		EndCounter(1);
		WriteScratchReg(0);

		StartCounter();
		// Start and check for termination of each accelerator.
		FFIInst.StartAcc();
		FFIInst.TerminateAcc();
		EndCounter(2);

		WriteScratchReg(0x8);
		StartCounter();
		// Read back output from IFFT.
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		EndCounter(3);
		WriteScratchReg(0);

		ChannelsLeft--;
	}
}

void FFIChain::PsychoNonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		WriteScratchReg(0x2);
		StartCounter();
		// Wait for FFT (consumer) to be ready.
		while (sm_sync[ConsRdyFlag] != 1);
		// Reset flag for next iteration.
		sm_sync[ConsRdyFlag] = 0;
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);
		WriteScratchReg(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		WriteScratchReg(0x4);
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
		WriteScratchReg(0);

		StartCounter();
		// Wait for IFFT (producer) to send output.
		while (sm_sync[ProdVldFlag] != 1);
		// Reset flag for next iteration.
		sm_sync[ProdVldFlag] = 0;
		EndCounter(2);

		WriteScratchReg(0x8);
		// Read back output from IFFT
		StartCounter();
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		// Inform IFFT (producer) - ready for next iteration.
		sm_sync[ProdRdyFlag] = 1;
		EndCounter(3);	
		WriteScratchReg(0);

		ChannelsLeft--;
	}
}

void FFIChain::BinaurRegularProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			WriteScratchReg(0x100);
			StartCounter();
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, false);
			EndCounter(4);
			WriteScratchReg(0);

			WriteScratchReg(0x200);
			StartCounter();
			// Write input data for FIR filters.
			InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - ChannelsLeft]);
			EndCounter(5);
			WriteScratchReg(0);

			// Start and check for termination of each accelerator.
			StartCounter();
			FFIInst.StartAcc();
			FFIInst.TerminateAcc();
			EndCounter(6);

			WriteScratchReg(0x400);
			StartCounter();
			// Read back output from IFFT
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			EndCounter(7);	
			WriteScratchReg(0);

			ChannelsLeft--;
		}
	}
}

void FFIChain::BinaurNonPipelineProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			WriteScratchReg(0x100);
			StartCounter();
			// Wait for FFT (consumer) to be ready.
			while (sm_sync[ConsRdyFlag] != 1);
			// Reset flag for next iteration.
			sm_sync[ConsRdyFlag] = 0;
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
			EndCounter(4);
			WriteScratchReg(0);

			WriteScratchReg(0x200);
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
			WriteScratchReg(0);

			StartCounter();
			// Wait for IFFT (producer) to send output.
			while (sm_sync[ProdVldFlag] != 1);
			// Reset flag for next iteration.
			sm_sync[ProdVldFlag] = 0;
			EndCounter(6);

			WriteScratchReg(0x400);
			StartCounter();
			// Read back output from IFFT
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			// Inform IFFT (producer) - ready for next iteration.
			sm_sync[ProdRdyFlag] = 1;
			EndCounter(7);	
			WriteScratchReg(0);

			ChannelsLeft--;
		}
	}
}

#endif // USE_MONOLITHIC_ACC

#endif // FFICHAINMONO_H
