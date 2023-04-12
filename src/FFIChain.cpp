#include <iostream>

#include <FFIThreadInfo.hpp>
#include <FFIChain.hpp>
#include <FFIChainHelper.hpp>
#include <FFIChainData.hpp>
#include <FFIChainDMA.hpp>
#include <FFIChainMono.hpp>

#if (USE_MONOLITHIC_ACC == 0)
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

	audio_fft_cfg_000[0].logn_samples = logn_samples;
	audio_fir_cfg_000[0].logn_samples = logn_samples;
	audio_ifft_cfg_000[0].logn_samples = logn_samples;

	audio_fft_cfg_000[0].esp.coherence = CoherenceMode;
	audio_fir_cfg_000[0].esp.coherence = CoherenceMode;
	audio_ifft_cfg_000[0].esp.coherence = CoherenceMode;

	audio_fft_cfg_000[0].spandex_conf = SpandexConfig.spandex_reg;
	audio_fir_cfg_000[0].spandex_conf = SpandexConfig.spandex_reg;
	audio_ifft_cfg_000[0].spandex_conf = SpandexConfig.spandex_reg;

	audio_ifft_cfg_000[0].src_offset = 2 * acc_size;
	audio_ifft_cfg_000[0].dst_offset = 2 * acc_size;

	audio_fir_cfg_000[0].src_offset = acc_size;
	audio_fir_cfg_000[0].dst_offset = acc_size;

    fft_cfg_000[0].hw_buf = mem;
    fir_cfg_000[0].hw_buf = mem;
    ifft_cfg_000[0].hw_buf = mem;

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
    esp_run(fft_cfg_000, 1);
    esp_run(fir_cfg_000, 1);
    esp_run(ifft_cfg_000, 1);
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
#endif

void FFIChain::PsychoProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned InputChannelsLeft = m_nChannelCount;
	unsigned FilterChannelsLeft = m_nChannelCount;
	unsigned OutputChannelsLeft = m_nChannelCount;

	// Check if any of the tasks are pending. If yes,
	// check if any of them a ready, based on a simple priority.
	// Once a task is complete, we reduce the number of tasks left
	// for that respective task.
	while (InputChannelsLeft != 0 || FilterChannelsLeft != 0 || OutputChannelsLeft != 0) {
		if (InputChannelsLeft) {
			// Wait for FFT (consumer) to be ready
			if (sm_sync[ConsRdyFlag] == 1) {
				sm_sync[ConsRdyFlag] = 0;
				// Write input data for FFT
        		StartCounter();
				InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, true);
    			asm volatile ("fence w, w");
        		EndCounter(0);
				// Inform FFT (consumer)
				sm_sync[ConsVldFlag] = 1;
				InputChannelsLeft--;
			}
		}

		if (FilterChannelsLeft) {
			// Wait for FIR (consumer) to be ready
			if (sm_sync[FltRdyFlag] == 1) {
				sm_sync[FltRdyFlag] = 0;

            	unsigned iChannelOrder = int(sqrt(m_nChannelCount - FilterChannelsLeft));

				// Write input data for filters
        		StartCounter();
				InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
    			asm volatile ("fence w, w");
        		EndCounter(1);
				// Inform FIR (consumer)
				sm_sync[FltVldFlag] = 1;
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

void FFIChain::BinaurProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
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
				if (sm_sync[FltRdyFlag] == 1) {
					sm_sync[FltRdyFlag] = 0;
					// Write input data for filters
        			StartCounter();
					InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - FilterChannelsLeft]);
    				asm volatile ("fence w, w");
        			EndCounter(4);
					// Inform FIR (consumer)
					sm_sync[FltVldFlag] = 1;
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

void FFIChain::StartCounter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (StartTime)
		:
		: "t0"
	);
}

void FFIChain::EndCounter(unsigned Index) {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (EndTime)
		:
		: "t0"
	);

    TotalTime[Index] += EndTime - StartTime;
}

void FFIChain::PrintTimeInfo(unsigned factor, bool isPsycho) {
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD) {
		printf("Init Data\t = %llu\n", TotalTime[0]/factor);
		printf("Init Filters\t = %llu\n", TotalTime[1]/factor);
		printf("Acc execution\t = %llu\n", TotalTime[2]/factor);
		printf("Output Read\t = %llu\n", TotalTime[3]/factor);
	} else if (DO_PP_CHAIN_OFFLOAD) {
		printf("Psycho Init Data\t = %llu\n", TotalTime[0]/factor);
		printf("Psycho Init Filters\t = %llu\n", TotalTime[1]/factor);
		printf("Psycho Acc execution\t = 0\n");
		printf("Psycho Output Read\t = %llu\n", TotalTime[2]/factor);
		printf("Binaur Init Data\t = %llu\n", TotalTime[3]/factor);
		printf("Binaur Init Filters\t = %llu\n", TotalTime[4]/factor);
		printf("Binaur Acc execution\t = 0\n");
		printf("Binaur Output Read\t = %llu\n", TotalTime[5]/factor);
	}
}
