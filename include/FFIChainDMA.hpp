#ifndef FFI_CHAIN_DMA
#define FFI_CHAIN_DMA

void FFIChain::ConfigureDMA(kiss_fft_cpx** m_ppcpPsychFilters, kiss_fft_cpx*** m_ppcpFilters) {
    dma_offset = 8 * acc_len;

	audio_dma_cfg_000[0].start_offset = dma_offset;
	audio_dma_cfg_000[0].esp.coherence = CoherenceMode;

	audio_dma_cfg_000[0].src_offset = 0;
	audio_dma_cfg_000[0].dst_offset = 0;

    dma_cfg_000[0].hw_buf = mem;

	// Zero out sync region.
	for (unsigned sync_index = 0; sync_index < 2 * SYNC_VAR_SIZE; sync_index++) {
		sm_sync[dma_offset + sync_index] = 0;
	}

	// Reset all sync variables to default values.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[dma_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 0;

	StartDMA();

	LoadAllData(m_ppcpPsychFilters, m_ppcpFilters);

	audio_dma_cfg_000[0].spandex_conf = SpandexConfig.spandex_reg;

	// Zero out sync region.
	for (unsigned sync_index = 0; sync_index < 2 * SYNC_VAR_SIZE; sync_index++) {
		sm_sync[dma_offset + sync_index] = 0;
	}

	// Reset all sync variables to default values.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[dma_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 0;

	StartDMA();
}

void FFIChain::StartDMA() {
    // Invoke accelerators but do not check for end
    audio_dma_cfg_000[0].esp.start_stop = 1;
    
    esp_run(dma_cfg_000, 1);
}

void FFIChain::EndDMA() {
	// Wait for DMA (consumer) to be ready.
	while (sm_sync[dma_offset + READY_FLAG_OFFSET] != 1);
	// Reset flag for next iteration.
	sm_sync[dma_offset + READY_FLAG_OFFSET] = 0;
	
	// End the DMA operation.
	sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 2;

	// Inform DMA (consumer) to start.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
}

void FFIChain::LoadAllData(kiss_fft_cpx** m_ppcpPsychFilters, kiss_fft_cpx*** m_ppcpFilters) {
    unsigned Psycho_ChannelCount = unsigned(sqrt(m_nChannelCount));

    // We're writing the input data to the same location.
	sm_sync[dma_offset + MEM_SRC_OFFSET] = dma_offset + 2 * SYNC_VAR_SIZE;

	// First, we load the input data for all 16 channels
	// into the DMA scratchpad.
	sm_sync[dma_offset + RD_SIZE] = m_nBlockSize;

    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
        // Wait for DMA (consumer) to be ready.
		while (sm_sync[dma_offset + READY_FLAG_OFFSET] != 1);

        // Reset flag for next iteration.
	    sm_sync[dma_offset + READY_FLAG_OFFSET] = 0;

        for(unsigned niSample = 0; niSample < m_nBlockSize; niSample++) {
            mem[dma_offset + (2 * SYNC_VAR_SIZE) + niSample] = FLOAT_TO_FIXED_WRAP(rand()/RAND_MAX, AUDIO_FX_IL);
        }

        // We increment the scratchpad offset every iteration.
	    sm_sync[dma_offset + RD_SP_OFFSET] = niChannel * m_nBlockSize;

	    // Inform DMA (consumer) to start.
	    sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
    }

    // Save the offset in the scratchpad for later.
    init_data_offset = m_nChannelCount * m_nBlockSize;

	// Next, we load the filter weights for the psycho-acoustic filter
	sm_sync[dma_offset + RD_SIZE] = 2 * m_nFFTBins;

    for(unsigned niChannel = 0; niChannel < Psycho_ChannelCount; niChannel++) {
        // Wait for DMA (consumer) to be ready.
		while (sm_sync[dma_offset + READY_FLAG_OFFSET] != 1);

        // Reset flag for next iteration.
	    sm_sync[dma_offset + READY_FLAG_OFFSET] = 0;

        for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
            mem[dma_offset + (2 * SYNC_VAR_SIZE) + (2 * niSample)] = FLOAT_TO_FIXED_WRAP(m_ppcpPsychFilters[niChannel][niSample].r, AUDIO_FX_IL);
            mem[dma_offset + (2 * SYNC_VAR_SIZE) + (2 * niSample + 1)] = FLOAT_TO_FIXED_WRAP(m_ppcpPsychFilters[niChannel][niSample].i, AUDIO_FX_IL);
        }

        // We increment the scratchpad offset every iteration.
	    sm_sync[dma_offset + RD_SP_OFFSET] = init_data_offset + niChannel * 2 * m_nFFTBins;

	    // Inform DMA (consumer) to start.
	    sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
    }

    // Save the offset in the scratchpad for later.
    psycho_filters_offset = init_data_offset + (Psycho_ChannelCount * 2 * m_nFFTBins);

	// Last, we load the filter weights for the binauralizer filter
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            // Wait for DMA (consumer) to be ready.
            while (sm_sync[dma_offset + READY_FLAG_OFFSET] != 1);
            // Reset flag for next iteration.
            sm_sync[dma_offset + READY_FLAG_OFFSET] = 0;

            for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
                mem[dma_offset + (2 * SYNC_VAR_SIZE) + (2 * niSample)] = FLOAT_TO_FIXED_WRAP(m_ppcpFilters[niEar][niChannel][niSample].r, AUDIO_FX_IL);
                mem[dma_offset + (2 * SYNC_VAR_SIZE) + (2 * niSample + 1)] = FLOAT_TO_FIXED_WRAP(m_ppcpFilters[niEar][niChannel][niSample].i, AUDIO_FX_IL);
            }

            // We increment the scratchpad offset every iteration.
	        sm_sync[dma_offset + RD_SP_OFFSET] = psycho_filters_offset + ((niEar * m_nChannelCount + niChannel) * 2 * m_nFFTBins);

	        // Inform DMA (consumer) to start.
	        sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
        }
    }
	
	EndDMA();
}

void FFIChain::StoreInputData(unsigned InitChannel) {
    // We're now storing data from the DMA's scratchpad.
    sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 1;

    // DMA will write the input data to the same
    // location for the FFT to read.
	sm_sync[dma_offset + MEM_DST_OFFSET] = SYNC_VAR_SIZE;

	// Size for each transfer is the same.
	sm_sync[dma_offset + WR_SIZE] = m_nBlockSize;

	// Offsets for sync variables to FFT.
	sm_sync[dma_offset + CONS_VALID_OFFSET] = VALID_FLAG_OFFSET;
	sm_sync[dma_offset + CONS_READY_OFFSET] = READY_FLAG_OFFSET;

    // We increment the scratchpad offset every iteration.
	sm_sync[dma_offset + WR_SP_OFFSET] = InitChannel * m_nBlockSize;

	// Inform DMA (consumer) to start.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
}

void FFIChain::StorePsychoFilters(unsigned InitChannel) {
    // We're now storing data from the DMA's scratchpad.
    sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 1;

    // DMA will write the input data to the same
    // location for the FFT to read.
	sm_sync[dma_offset + MEM_DST_OFFSET] = 5 * acc_len;

	// Size for each transfer is the same.
	sm_sync[dma_offset + WR_SIZE] = 2 * m_nFFTBins;

	// Offsets for sync variables to FFT.
	sm_sync[dma_offset + CONS_VALID_OFFSET] = acc_len + FLT_VALID_FLAG_OFFSET;
	sm_sync[dma_offset + CONS_READY_OFFSET] = acc_len + FLT_READY_FLAG_OFFSET;

    // We increment the scratchpad offset every iteration.
	sm_sync[dma_offset + WR_SP_OFFSET] = init_data_offset + InitChannel * 2 * m_nFFTBins;

	// Inform DMA (consumer) to start.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
}

void FFIChain::StoreBinaurFilters(unsigned InitEar, unsigned InitChannel) {
    // We're now storing data from the DMA's scratchpad.
    sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 1;

    // DMA will write the input data to the same
    // location for the FFT to read.
	sm_sync[dma_offset + MEM_DST_OFFSET] = 5 * acc_len;

	// Size for each transfer is the same.
	sm_sync[dma_offset + WR_SIZE] = 2 * m_nFFTBins;

	// Offsets for sync variables to FFT.
	sm_sync[dma_offset + CONS_VALID_OFFSET] = acc_len + FLT_VALID_FLAG_OFFSET;
	sm_sync[dma_offset + CONS_READY_OFFSET] = acc_len + FLT_READY_FLAG_OFFSET;

    // We increment the scratchpad offset every iteration.
	sm_sync[dma_offset + WR_SP_OFFSET] = psycho_filters_offset + (InitEar * m_nChannelCount + InitChannel) * 2 * m_nFFTBins;

	// Inform DMA (consumer) to start.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
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
				StoreInputData(m_nChannelCount - InputChannelsLeft);

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
				StorePsychoFilters(iChannelOrder);

				FilterChannelsLeft--;
			}
		}

		if (OutputChannelsLeft) {
			// Wait for IFFT (producer) to send output
			if (sm_sync[ProdVldFlag] == 1) {
				sm_sync[ProdVldFlag] = 0;
				// Read back output
        		StartCounter();
				PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - OutputChannelsLeft);
    			asm volatile ("fence w, w");
        		EndCounter(2);
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
        			StartCounter();
					InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, false);
    				asm volatile ("fence w, w");
        			EndCounter(3);
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
					StoreBinaurFilters(niEar, m_nChannelCount - FilterChannelsLeft);

					FilterChannelsLeft--;
				}
			}

			if (OutputChannelsLeft) {
				// Wait for IFFT (producer) to send output
				if (sm_sync[ProdVldFlag] == 1) {
					sm_sync[ProdVldFlag] = 0;
					// Read back output
        			StartCounter();
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1), (OutputChannelsLeft == m_nChannelCount));
    				asm volatile ("fence w, w");
        			EndCounter(5);
					// Inform IFFT (producer)
					sm_sync[ProdRdyFlag] = 1;
					OutputChannelsLeft--;
				}
			}
		}
	}
}

#endif // FFI_CHAIN_DMA