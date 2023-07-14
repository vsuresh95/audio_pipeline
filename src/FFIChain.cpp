#include <iostream>

#include <FFIChain.hpp>
#include <FFIThreadInfo.hpp>
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

    fft_cfg_000[0].hw_buf = mem;
    fir_cfg_000[0].hw_buf = mem;
    ifft_cfg_000[0].hw_buf = mem;

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

void FFIChain::FFTRegularProcess(kiss_fft_scalar* timedata, kiss_fft_scalar* freqdata) {
	// Write input data for FFT.
	unsigned InitLength = m_nFFTSize;
	audio_token_t SrcData_i;
	audio_t* src_i;
	device_token_t DstData_i;
	device_t* dst_i;

	// See init_params() for memory layout.
	src_i = timedata;
	dst_i = mem + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src_i+=2, dst_i+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData_i.value_64 = read_mem_reqv((void *) src_i);

		DstData_i.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_1, AUDIO_FX_IL);
		DstData_i.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst_i, DstData_i.value_64);
	}
	EndCounter(0);

	// Start and check for termination of each accelerator.
	StartCounter();
	esp_run(fft_cfg_000, 1);
	EndCounter(1);

	unsigned ReadLength = m_nFFTSize;

	device_token_t SrcData_o;
	device_t* src_o;
	audio_token_t DstData_o;
	audio_t* dst_o;

	// See init_params() for memory layout.
	src_o = mem + (1 * acc_len) + SYNC_VAR_SIZE;
	dst_o = freqdata;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for (unsigned niSample = 0; niSample < ReadLength; niSample+=2, src_o+=2, dst_o+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData_o.value_64 = read_mem_reqodata((void *) src_o);

		DstData_o.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_1, AUDIO_FX_IL);
		DstData_o.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst_o, DstData_o.value_64);
	}
	EndCounter(2);
}

void FFIChain::IFFTRegularProcess(kiss_fft_scalar* timedata, kiss_fft_scalar* freqdata) {
	// Write input data for FFT.
	unsigned InitLength = m_nFFTSize;
	audio_token_t SrcData_i;
	audio_t* src_i;
	device_token_t DstData_i;
	device_t* dst_i;

	// See init_params() for memory layout.
	src_i = freqdata;
	dst_i = mem + (2 * acc_len) + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src_i+=2, dst_i+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData_i.value_64 = read_mem_reqv((void *) src_i);

		DstData_i.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_1, AUDIO_FX_IL);
		DstData_i.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst_i, DstData_i.value_64);
	}
	EndCounter(0);

	// Start and check for termination of each accelerator.
	StartCounter();
	esp_run(ifft_cfg_000, 1);
	EndCounter(1);

	unsigned ReadLength = m_nFFTSize;
	device_token_t SrcData_o;
	device_t* src_o;
	audio_token_t DstData_o;
	audio_t* dst_o;

	// See init_params() for memory layout.
	src_o = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
	dst_o = timedata;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for (unsigned niSample = 0; niSample < ReadLength; niSample+=2, src_o+=2, dst_o+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData_o.value_64 = read_mem_reqodata((void *) src_o);

		DstData_o.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_1, AUDIO_FX_IL);
		DstData_o.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst_o, DstData_o.value_64);
	}
	EndCounter(2);
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
		esp_run(fft_cfg_000, 1);
		esp_run(fir_cfg_000, 1);
		esp_run(ifft_cfg_000, 1);
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
			esp_run(fft_cfg_000, 1);
			esp_run(fir_cfg_000, 1);
			esp_run(ifft_cfg_000, 1);
			EndCounter(6);

			// Read back output from IFFT
			StartCounter();
			// BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount), m_nChannelCount - ChannelsLeft);
			EndCounter(7);	

			ChannelsLeft--;
		}
	}
}
#endif

void FFIChain::PsychoNonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		StartCounter();
		// Wait for FFT (consumer) to be ready.
		SpinSync(ConsRdyFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ConsRdyFlag, 0);
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		StartCounter();
		// Wait for FIR (consumer) to be ready.
		SpinSync(FltRdyFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(FltRdyFlag, 0);
		// Write input data for FIR filters.
		InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
		// Inform FIR (consumer) of filters ready.
		UpdateSync(FltVldFlag, 1);
		// Inform FFT (consumer) to start.
		UpdateSync(ConsVldFlag, 1);
		EndCounter(1);

		StartCounter();
		// Wait for IFFT (producer) to send output.
		SpinSync(ProdVldFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ProdVldFlag, 0);
		EndCounter(2);

		// Read back output from IFFT
		StartCounter();
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		// Inform IFFT (producer) - ready for next iteration.
		UpdateSync(ProdRdyFlag, 1);
		EndCounter(3);	

		ChannelsLeft--;
	}
}

void FFIChain::BinaurNonPipelineProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			StartCounter();
			// Wait for FFT (consumer) to be ready.
			SpinSync(ConsRdyFlag, 1);
			// Reset flag for next iteration.
			UpdateSync(ConsRdyFlag, 0);
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
			EndCounter(4);

			StartCounter();
			// Wait for FIR (consumer) to be ready.
			SpinSync(FltRdyFlag, 1);
			// Reset flag for next iteration.
			UpdateSync(FltRdyFlag, 0);
			// Write input data for FIR filters.
			InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - ChannelsLeft]);
			// Inform FIR (consumer) of filters ready.
			UpdateSync(FltVldFlag, 1);
			// Inform FFT (consumer) to start.
			UpdateSync(ConsVldFlag, 1);
			EndCounter(5);			

			StartCounter();
			// Wait for IFFT (producer) to send output.
			SpinSync(ProdVldFlag, 1);
			// Reset flag for next iteration.
			UpdateSync(ProdVldFlag, 0);
			EndCounter(6);

			// Read back output from IFFT
			StartCounter();
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			// Inform IFFT (producer) - ready for next iteration.
			UpdateSync(ProdRdyFlag, 1);
			EndCounter(7);	

			ChannelsLeft--;
		}
	}
}

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
			if (TestSync(ConsRdyFlag, 1)) {
				UpdateSync(ConsRdyFlag, 0);
				// Write input data for FFT
        		StartCounter();
				InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, true);
        		EndCounter(0);
				// Inform FFT (consumer)
				UpdateSync(ConsVldFlag, 1);
				InputChannelsLeft--;
			}
		}

		if (FilterChannelsLeft) {
			// Wait for FIR (consumer) to be ready
			if (TestSync(FltRdyFlag, 1)) {
				UpdateSync(FltRdyFlag, 0);

            	unsigned iChannelOrder = int(sqrt(m_nChannelCount - FilterChannelsLeft));

				// Write input data for filters
        		StartCounter();
				InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
        		EndCounter(1);
				// Inform FIR (consumer)
				UpdateSync(FltVldFlag, 1);
				FilterChannelsLeft--;
			}
		}

		if (OutputChannelsLeft) {
			// Wait for IFFT (producer) to send output
			if (TestSync(ProdVldFlag, 1)) {
				UpdateSync(ProdVldFlag, 0);
				// Read back output
        		StartCounter();
				PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - OutputChannelsLeft);
        		EndCounter(2);
				// Inform IFFT (producer)
				UpdateSync(ProdRdyFlag, 1);
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
				if (TestSync(ConsRdyFlag, 1)) {
					UpdateSync(ConsRdyFlag, 0);
					// Write input data for FFT
        			StartCounter();
					InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, false);
        			EndCounter(3);
					// Inform FFT (consumer)
					UpdateSync(ConsVldFlag, 1);
					InputChannelsLeft--;
				}
			}

			if (FilterChannelsLeft) {
				// Wait for FIR (consumer) to be ready
				if (TestSync(FltRdyFlag, 1)) {
					UpdateSync(FltRdyFlag, 0);
					// Write input data for filters
        			StartCounter();
					InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - FilterChannelsLeft]);
        			EndCounter(4);
					// Inform FIR (consumer)
					UpdateSync(FltVldFlag, 1);
					FilterChannelsLeft--;
				}
			}

			if (OutputChannelsLeft) {
				// Wait for IFFT (producer) to send output
				if (TestSync(ProdVldFlag, 1)) {
					UpdateSync(ProdVldFlag, 0);
					// Read back output
        			StartCounter();
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1), (OutputChannelsLeft == m_nChannelCount));
        			EndCounter(5);
					// Inform IFFT (producer)
					UpdateSync(ProdRdyFlag, 1);
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
		printf("Psycho Init Data\t = %llu\n", TotalTime[0]/factor);
		printf("Psycho Init Filters\t = %llu\n", TotalTime[1]/factor);
		printf("Psycho Acc execution\t = %llu\n", TotalTime[2]/factor);
		printf("Psycho Output Read\t = %llu\n", TotalTime[3]/factor);
		printf("Binaur Init Data\t = %llu\n", TotalTime[4]/factor);
		printf("Binaur Init Filters\t = %llu\n", TotalTime[5]/factor);
		printf("Binaur Acc execution\t = %llu\n", TotalTime[6]/factor);
		printf("Binaur Output Read\t = %llu\n", TotalTime[7]/factor);
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
