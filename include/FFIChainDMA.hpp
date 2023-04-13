#ifndef FFI_CHAIN_DMA
#define FFI_CHAIN_DMA

void FFIChain::ConfigureDMA() {
	// Find Audio DMA devices using ESP's probe function.
	DMAInst.ProbeAcc(0);

	// Assign memory parameters
	DMAInst.mem = mem;
	DMAInst.ptable = ptable;
	DMAInst.mem_size = mem_size;
	DMAInst.acc_len = acc_len;
	DMAInst.acc_size = acc_size;

    DMAInst.sm_sync = sm_sync;
    DMAInst.dma_offset = 8 * acc_len;

	// Assign size parameters
	DMAInst.m_nChannelCount = m_nChannelCount;
	DMAInst.m_nBlockSize = m_nBlockSize;
	DMAInst.m_nFFTBins = m_nFFTBins;

	DMAInst.CoherenceMode = CoherenceMode;

	DMAInst.ConfigureAcc();
	DMAInst.StartAcc();
	DMAInst.LoadAllData();
	DMAInst.TerminateAcc();

	DMAInst.SpandexReg = SpandexConfig.spandex_reg;

	DMAInst.ConfigureAcc();
	DMAInst.ConfigureSPXRegister();
	DMAInst.StartAcc();
}

void FFIChain::PsychoProcessDMA(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned InputChannelsLeft = m_nChannelCount;
	unsigned FilterChannelsLeft = m_nChannelCount;
	unsigned OutputChannelsLeft = m_nChannelCount;

	// Check if any of the tasks are pending. If yes,
	// check if any of them a ready, based on a simple priority.
	// Once a task is complete, we reduce the number of tasks left
	// for that respective task.
	while (InputChannelsLeft != 0 || FilterChannelsLeft != 0 || OutputChannelsLeft != 0) {
		// Wait for DMA to be ready
		if (InputChannelsLeft) {
			// Wait for FFT (consumer) to be ready
			if (sm_sync[ConsRdyFlag] == 1 && sm_sync[DMARdyFlag] == 1) {
				sm_sync[ConsRdyFlag] = 0;
				sm_sync[DMARdyFlag] = 0;

				// Offload filter weights loading process to DMA
				DMAInst.StoreInputData(m_nChannelCount - InputChannelsLeft);

				InputChannelsLeft--;
			}
		}

		if (FilterChannelsLeft) {
			// Wait for FFT (consumer) to be ready
			if (sm_sync[FltRdyFlag] == 1 && sm_sync[DMARdyFlag] == 1) {
				sm_sync[FltRdyFlag] = 0;
				sm_sync[DMARdyFlag] = 0;

  	         	unsigned iChannelOrder = unsigned(sqrt(m_nChannelCount - FilterChannelsLeft));

				// Offload filter weights loading process to DMA
				DMAInst.StorePsychoFilters(iChannelOrder);

				FilterChannelsLeft--;
			}
		}

		if (OutputChannelsLeft) {
			// Wait for IFFT (producer) to send output
			if (sm_sync[ProdVldFlag] == 1) {
				sm_sync[ProdVldFlag] = 0;
				// Read back output
				WriteScratchReg(0x8);
        		StartCounter();
				PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - OutputChannelsLeft);
    			asm volatile ("fence w, w");
        		EndCounter(2);
				WriteScratchReg(0);
				// Inform IFFT (producer)
				sm_sync[ProdRdyFlag] = 1;
				OutputChannelsLeft--;
			}
		}
	}
}

void FFIChain::BinaurProcessDMA(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
	// Check if any of the tasks are pending. If yes,
	// check if any of them a ready, based on a simple priority.
	// Once a task is complete, we reduce the number of tasks left
	// for that respective task.
	// Repeat this for both ears.
	// There is scope for optimization here because the inputs don't change.
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned InputChannelsLeft = m_nChannelCount;
		unsigned FilterChannelsLeft = m_nChannelCount;
		unsigned OutputChannelsLeft = m_nChannelCount;

		while (InputChannelsLeft != 0 || FilterChannelsLeft != 0 || OutputChannelsLeft != 0) {
			if (InputChannelsLeft) {
				// Wait for FFT (consumer) to be ready
				if (sm_sync[ConsRdyFlag] == 1) {
					sm_sync[ConsRdyFlag] = 0;
					// Write input data for FFT
					WriteScratchReg(0x100);
        			StartCounter();
					InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, false);
    				asm volatile ("fence w, w");
        			EndCounter(3);
					WriteScratchReg(0);
					// Inform FFT (consumer)
					sm_sync[ConsVldFlag] = 1;
					InputChannelsLeft--;
				}
			}

			if (FilterChannelsLeft) {
				// Wait for FIR (consumer) to be ready
				if (sm_sync[FltRdyFlag] == 1 && sm_sync[DMARdyFlag] == 1) {
					sm_sync[FltRdyFlag] = 0;
					sm_sync[DMARdyFlag] = 0;

					// Offload filter weights loading process to DMA
					DMAInst.StoreBinaurFilters(niEar, m_nChannelCount - FilterChannelsLeft);

					FilterChannelsLeft--;
				}
			}

			if (OutputChannelsLeft) {
				// Wait for IFFT (producer) to send output
				if (sm_sync[ProdVldFlag] == 1) {
					sm_sync[ProdVldFlag] = 0;
					// Read back output
					WriteScratchReg(0x400);
        			StartCounter();
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1), (OutputChannelsLeft == m_nChannelCount));
    				asm volatile ("fence w, w");
        			EndCounter(5);
					WriteScratchReg(0);
					// Inform IFFT (producer)
					sm_sync[ProdRdyFlag] = 1;
					OutputChannelsLeft--;
				}
			}
		}
	}
}

#endif // FFI_CHAIN_DMA