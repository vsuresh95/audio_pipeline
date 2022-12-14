#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <FFIChain.hpp>

// Helper function to perform memory write using Spandex request types.
// By default, the writes are regular stores.
// See CohDefines.hpp for definition of WRITE_CODE.
static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

// Helper function to perform memory read using Spandex request types.
// By default, the reads are regular loads.
// See CohDefines.hpp for definition of READ_CODE.
static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

void FFIChain::InitParams() {
	// Number of samples for the FFT or FIR accelerator. (1024)
    num_samples = 1 << logn_samples;

	// Total number of samples will be 2x due to complex number pairs.
	// Size is calculated for the data type that the device works on.
	// Here, device_t can be int (4B) or fixed-point (4B).
	in_len = 2 * num_samples;
	out_len = 2 * num_samples;
	in_size = in_len * sizeof(device_t);
	out_size = out_len * sizeof(device_t);

	// We add the size of control partition to determine total number
	// of elements for each accelerator.
    acc_len = SYNC_VAR_SIZE + out_len;
    acc_size = acc_len * sizeof(device_t);

	// Total size of data for each accelerator
	mem_size = acc_size;

	// Total size for all accelerators combined:
	// 0*acc_size - 1*acc_size: FFT Input
	// 1*acc_size - 2*acc_size: FIR Input
	// 2*acc_size - 3*acc_size: IFFT Input
	// 3*acc_size - 4*acc_size: CPU Input
	// 4*acc_size - 5*acc_size: Unused
	// 5*acc_size - 7*acc_size: FIR filters
	// 7*acc_size - 7*acc_size: Twiddle factors
	// Therefore, NUM_DEVICES+5 = 8.
    mem_size = acc_size * NUM_DEVICES+5;

	// Helper flags for sync flags that the CPU needs to access.
	// We add a pair of sync flags for FIR filter weights, so that
	// this write can be overlapped with FFT operations.
	ConsRdyFlag = 0*acc_len + READY_FLAG_OFFSET;
	ConsVldFlag = 0*acc_len + VALID_FLAG_OFFSET;
	FltRdyFlag = 1*acc_len + FLT_READY_FLAG_OFFSET;
	FltVldFlag = 1*acc_len + FLT_VALID_FLAG_OFFSET;
	ProdRdyFlag = 3*acc_len + READY_FLAG_OFFSET;
	ProdVldFlag = 3*acc_len + VALID_FLAG_OFFSET;
}

void FFIChain::ConfigureAcc() {
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
	FFTInst.ProbeAcc(0);
	FIRInst.ProbeAcc(0);
	IFFTInst.ProbeAcc(1);

	// Assign parameters to all accelerator objects.
	FFTInst.logn_samples = FIRInst.logn_samples = IFFTInst.logn_samples = logn_samples;
	FFTInst.ptable = FIRInst.ptable = IFFTInst.ptable = ptable;
    FFTInst.mem_size = FIRInst.mem_size = IFFTInst.mem_size = mem_size;
    FFTInst.acc_size = FIRInst.acc_size = IFFTInst.acc_size = acc_size;
    FFTInst.SpandexReg = FIRInst.SpandexReg = IFFTInst.SpandexReg = SpandexConfig.spandex_reg;
    FFTInst.CoherenceMode =  FIRInst.CoherenceMode = IFFTInst.CoherenceMode = CoherenceMode;
	IFFTInst.inverse = 1;

	// After assigning the parameters, we call ConfigureAcc() for
	// each accelerator object to program accelerator register
	// with these parameters.
	FFTInst.ConfigureAcc();
	FIRInst.ConfigureAcc();
	IFFTInst.ConfigureAcc();

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
	FFTInst.StartAcc();
	FIRInst.StartAcc();
	IFFTInst.StartAcc();
}

void FFIChain::RegularProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel) {
	// Write input data for FFT.
	InitData(pBFSrcDst, CurChannel);
	// Write input data for FIR filters.
	InitFilters(pBFSrcDst, m_Filters);

	// Start and check for termination of each accelerator.
	FFTInst.StartAcc();
	FFTInst.TerminateAcc();
	FIRInst.StartAcc();
	FIRInst.TerminateAcc();
	IFFTInst.StartAcc();
	IFFTInst.TerminateAcc();

	// Read back output from IFFT.
	ReadOutput(pBFSrcDst, m_pfScratchBufferA, CurChannel);
}

void FFIChain::NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel) {
	// Wait for FFT (consumer) to be ready.
	while (sm_sync[ConsRdyFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[ConsRdyFlag] = 0;
	// Write input data for FFT.
	InitData(pBFSrcDst, CurChannel);
	// Inform FFT (consumer) to start.
	sm_sync[ConsVldFlag] = 1;

	// Wait for FIR (consumer) to be ready.
	while (sm_sync[FltRdyFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[FltRdyFlag] = 0;
	// Write input data for FIR filters.
	InitFilters(pBFSrcDst, m_Filters);
	// Inform FIR (consumer) to start.
	sm_sync[FltVldFlag] = 1;

	// Wait for IFFT (producer) to send output.
	while (sm_sync[ProdVldFlag] != 1);
	// Reset flag for next iteration.
	sm_sync[ProdVldFlag] = 0;
	// Read back output from IFFT
	ReadOutput(pBFSrcDst, m_pfScratchBufferA, CurChannel);
	// Inform IFFT (producer) - ready for next iteration.
	sm_sync[ProdRdyFlag] = 1;
}

void FFIChain::InitData(CBFormat* pBFSrcDst, unsigned InitChannel) {
	int InitLength = 2 * (pBFSrcDst->m_nSamples);
	int niChannel = pBFSrcDst->m_nChannelCount - InitChannel;
	spandex_token_t CoalescedData;
	device_t* dst;

	// See init_params() for memory layout.
	dst = mem + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, dst+=2)
	{
		CoalescedData.value_32_1 = FLOAT_TO_FIXED_WRAP(pBFSrcDst->m_ppfChannels[niChannel][niSample], AUDIO_FX_IL);
		CoalescedData.value_32_2 = FLOAT_TO_FIXED_WRAP(pBFSrcDst->m_ppfChannels[niChannel][niSample+1], AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, CoalescedData.value_64);
	}
}

void FFIChain::InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters) {
	int InitLength = 2 * (pBFSrcDst->m_nSamples + 1);
	spandex_token_t CoalescedData;
	device_t* dst;

	// See init_params() for memory layout.
	dst = mem + (5 * acc_len) + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample++, dst+=2)
	{
		CoalescedData.value_32_1 = FLOAT_TO_FIXED_WRAP(m_Filters[niSample].r, AUDIO_FX_IL);
		CoalescedData.value_32_2 = FLOAT_TO_FIXED_WRAP(m_Filters[niSample].i, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, CoalescedData.value_64);
	}
}

void FFIChain::ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA, unsigned ReadChannel) {
	int ReadLength =  2 * (pBFSrcDst->m_nSamples);
	int niChannel = pBFSrcDst->m_nChannelCount - ReadChannel;
	spandex_token_t CoalescedData;
	device_t* dst;

	// See init_params() for memory layout.
	dst = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < ReadLength; niSample++, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		CoalescedData.value_64 = read_mem((void *) dst);

		m_pfScratchBufferA[niSample] = FIXED_TO_FLOAT_WRAP(CoalescedData.value_32_1, AUDIO_FX_IL);
		m_pfScratchBufferA[niSample+1] = FIXED_TO_FLOAT_WRAP(CoalescedData.value_32_2, AUDIO_FX_IL);
	}
}

void FFIChain::InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles) {
	int InitLength = pBFSrcDst->m_nSamples;
	spandex_token_t CoalescedData;
	device_t* dst;

	// See init_params() for memory layout.
	dst = mem + (7 * acc_len) + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample++, dst+=2)
	{
		CoalescedData.value_32_1 = FLOAT_TO_FIXED_WRAP(super_twiddles[niSample].r, AUDIO_FX_IL);
		CoalescedData.value_32_2 = FLOAT_TO_FIXED_WRAP(super_twiddles[niSample].i, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, CoalescedData.value_64);
	}
}

void FFIChain::SetSpandexConfig(unsigned UseESP, unsigned CohPrtcl) {
	if (UseESP) {
		SpandexConfig.spandex_reg = 0;
	} else {
		if (CohPrtcl == SPX_OWNER_PREDICTION) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 2;
			SpandexConfig.w_en = 1;
			SpandexConfig.w_op = 1;
			SpandexConfig.w_type = 1;
		} else if (CohPrtcl == SPX_WRITE_THROUGH_FWD) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 2;
			SpandexConfig.w_en = 1;
			SpandexConfig.w_type = 1;
		} else if (CohPrtcl == SPX_BASELINE_REQV) {
			SpandexConfig.spandex_reg = 0;
			SpandexConfig.r_en = 1;
			SpandexConfig.r_type = 1;
		} else {
			SpandexConfig.spandex_reg = 0;
		}
	}
}

void FFIChain::SetCohMode(unsigned UseESP, unsigned CohPrtcl) {
	if (!UseESP) {
		CoherenceMode = ACC_COH_FULL;
	} else {
		if (CohPrtcl == ESP_NON_COHERENT_DMA) {
			CoherenceMode = ACC_COH_NONE;
		} else if (CohPrtcl == ESP_LLC_COHERENT_DMA) {
			CoherenceMode = ACC_COH_LLC;
		} else if (CohPrtcl == ESP_COHERENT_DMA) {
			CoherenceMode = ACC_COH_RECALL;
		} else {
			CoherenceMode = ACC_COH_FULL;
		}
	}    
}

// void FFIChain::PipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters) {
// 	unsigned InputChannelsLeft = pBFSrcDst->m_nChannelCount;
// 	unsigned FilterChannelsLeft = pBFSrcDst->m_nChannelCount;
// 	unsigned OutputChannelsLeft = pBFSrcDst->m_nChannelCount;
// 
// 	while (InputChannelsLeft != 0 && FilterChannelsLeft != 0 && OutputChannelsLeft != 0) {
// 		if (InputChannelsLeft) {
// 			// Wait for FFT (consumer) to be ready
// 			if (sm_sync[ConsRdyFlag] == 1) {
// 				sm_sync[ConsRdyFlag] = 0;
// 				// Write input data for FFT
// 				InitData(pBFSrcDst, InputChannelsLeft);
// 				// Inform FFT (consumer)
// 				sm_sync[ConsVldFlag] = 1;
// 				InputChannelsLeft--;
// 			}
// 		}
// 
// 		if (FilterChannelsLeft) {
// 			// Wait for FIR (consumer) to be ready
// 			if (sm_sync[FltRdyFlag] == 1) {
// 				sm_sync[FltRdyFlag] = 0;
// 				// Write input data for filters
// 				InitFilters(pBFSrcDst, m_Filters[FilterChannelsLeft]);
// 				// Inform FIR (consumer)
// 				sm_sync[FltVldFlag] = 1;
// 				FilterChannelsLeft--;
// 			}
// 		}
// 
// 		if (OutputChannelsLeft) {
// 			// Wait for IFFT (producer) to send output
// 			if (sm_sync[ProdVldFlag] == 1) {
// 				sm_sync[ProdVldFlag] = 0;
// 				// Read back output
// 				ReadOutput(pBFSrcDst, OutputChannelsLeft);
// 				// Inform IFFT (producer)
// 				sm_sync[ProdRdyFlag] = 1;
// 				OutputChannelsLeft--;
// 			}
// 		}
// 	}
// }