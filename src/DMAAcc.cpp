#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <AccDefines.hpp>
#include <CohDefines.hpp>

#include <DMAAcc.hpp>

#include <math.h>

void DMAAcc::ProbeAcc(int DeviceNum) {
    nDev = probe(&espdevs, VENDOR_SLD, SLD_AUDIO_DMA, DMA_DEV_NAME);
	if (nDev == 0) {
		printf("%s not found\n", DMA_DEV_NAME);
		return;
	}

	DMADev = &espdevs[DeviceNum];
}

void DMAAcc::ConfigureAcc() {
	// Pass common configuration parameters
	iowrite32(DMADev, SELECT_REG, ioread32(DMADev, DEVID_REG));
	iowrite32(DMADev, COHERENCE_REG, CoherenceMode);

	iowrite32(DMADev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(DMADev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(DMADev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(DMADev, SRC_OFFSET_REG, 0);
	iowrite32(DMADev, DST_OFFSET_REG, 0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(DMADev, AUDIO_DMA_START_OFFSET_REG, 8 * acc_len);

	// Zero out sync region.
	for (unsigned sync_index = 0; sync_index < 2 * SYNC_VAR_SIZE; sync_index++) {
		sm_sync[dma_offset + sync_index] = 0;
	}

	// Reset all sync variables to default values.
	sm_sync[dma_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[dma_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 0;
}

void DMAAcc::ConfigureSPXRegister() {
	iowrite32(DMADev, SPANDEX_REG, SpandexReg);
}

void DMAAcc::StartAcc() {
	// Start accelerators
	iowrite32(DMADev, CMD_REG, CMD_MASK_START);
}

void DMAAcc::TerminateAcc() {
    unsigned done;

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(DMADev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}

	iowrite32(DMADev, CMD_REG, 0x0);
}

void DMAAcc::LoadAllData() {
    unsigned Psycho_ChannelCount = sqrt(m_nChannelCount);

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

		#if (DO_DATA_INIT == 1)
        for(unsigned niSample = 0; niSample < m_nBlockSize; niSample++) {
            mem[dma_offset + (2 * SYNC_VAR_SIZE) + niSample] = FLOAT_TO_FIXED_WRAP(myRand(), AUDIO_FX_IL);
        }
		#endif

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

		#if (DO_DATA_INIT == 1)
        for(unsigned niSample = 0; niSample < 2 * m_nFFTBins; niSample++) {
            mem[dma_offset + (2 * SYNC_VAR_SIZE) + niSample] = FLOAT_TO_FIXED_WRAP(myRand(), AUDIO_FX_IL);
        }
		#endif

        // We increment the scratchpad offset every iteration.
	    sm_sync[dma_offset + RD_SP_OFFSET] = init_data_offset + niChannel * 2 * m_nFFTBins;

	    // Inform DMA (consumer) to start.
	    sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
    }

    // Save the offset in the scratchpad for later.
    psycho_filters_offset = init_data_offset + Psycho_ChannelCount * 2 * m_nFFTBins;

	// Last, we load the filter weights for the binauralizer filter
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            // Wait for DMA (consumer) to be ready.
            while (sm_sync[dma_offset + READY_FLAG_OFFSET] != 1);
            // Reset flag for next iteration.
            sm_sync[dma_offset + READY_FLAG_OFFSET] = 0;

			#if (DO_DATA_INIT == 1)
            for(unsigned niSample = 0; niSample < 2 * m_nFFTBins; niSample++) {
                mem[dma_offset + (2 * SYNC_VAR_SIZE) + niSample] = FLOAT_TO_FIXED_WRAP(myRand(), AUDIO_FX_IL);
            }
			#endif

            // We increment the scratchpad offset every iteration.
	        sm_sync[dma_offset + RD_SP_OFFSET] = psycho_filters_offset + ((niEar * m_nChannelCount + niChannel) * 2 * m_nFFTBins);

			if (niEar == 1 && niChannel == m_nChannelCount - 1) {
				sm_sync[dma_offset + LOAD_STORE_FLAG_OFFSET] = 2;
			}

	        // Inform DMA (consumer) to start.
	        sm_sync[dma_offset + VALID_FLAG_OFFSET] = 1;
        }
    }
}

void DMAAcc::StoreInputData(unsigned InitChannel) {
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

void DMAAcc::StorePsychoFilters(unsigned InitChannel) {
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

void DMAAcc::StoreBinaurFilters(unsigned InitEar, unsigned InitChannel) {
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