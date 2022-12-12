#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <FFIChain.hpp>

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
    num_samples = 1 << logn_samples;

	printf("logn_samples = %d\n", logn_samples);
	printf("num_samples = %d\n", num_samples);

	in_len = 2 * num_samples;
	out_len = 2 * num_samples;
	in_size = in_len * sizeof(audio_t);
	out_size = out_len * sizeof(audio_t);

	out_offset  = 0;
	mem_size = (out_offset * sizeof(audio_t)) + out_size + (SYNC_VAR_SIZE * sizeof(audio_t));

    acc_size = mem_size;
    acc_offset = out_offset + out_len + SYNC_VAR_SIZE;

	printf("acc_size = %d\n", acc_size);
	printf("acc_offset = %d\n", acc_offset);

    mem_size *= NUM_DEVICES+5;

	ConsRdyFlag = 0*acc_offset + READY_FLAG_OFFSET;
	ConsVldFlag = 0*acc_offset + VALID_FLAG_OFFSET;
	FltRdyFlag = 1*acc_offset + FLT_READY_FLAG_OFFSET;
	FltVldFlag = 1*acc_offset + FLT_VALID_FLAG_OFFSET;
	ProdRdyFlag = 3*acc_offset + READY_FLAG_OFFSET;
	ProdVldFlag = 3*acc_offset + VALID_FLAG_OFFSET;
}

void FFIChain::ConfigureAcc() {
	InitParams();

	// Allocate memory pointers
	mem = (audio_t *) aligned_malloc(mem_size);
    sm_sync = (volatile audio_t*) mem;

	printf("mem = %p\n", mem);

	// Allocate and populate page table
	ptable = (unsigned **) aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (unsigned i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(audio_t))];

	SetSpandexConfig(IS_ESP, COH_MODE);
	SetCohMode(IS_ESP, COH_MODE);

	FFTInst.ProbeAcc(0);
	FIRInst.ProbeAcc(0);
	IFFTInst.ProbeAcc(1);

	FFTInst.logn_samples = logn_samples;
	FFTInst.ptable = ptable;
    FFTInst.mem_size = mem_size;
    FFTInst.acc_size = acc_size;
    FFTInst.SpandexReg = SpandexConfig.spandex_reg;
    FFTInst.CoherenceMode = CoherenceMode;

	FIRInst.logn_samples = logn_samples;
	FIRInst.ptable = ptable;
    FIRInst.mem_size = mem_size;
    FIRInst.acc_size = acc_size;
    FIRInst.SpandexReg = SpandexConfig.spandex_reg;
    FIRInst.CoherenceMode = CoherenceMode;
	
	IFFTInst.logn_samples = logn_samples;
	IFFTInst.ptable = ptable;
    IFFTInst.mem_size = mem_size;
    IFFTInst.acc_size = acc_size;
    IFFTInst.SpandexReg = SpandexConfig.spandex_reg;
    IFFTInst.CoherenceMode = CoherenceMode;
	IFFTInst.inverse = 1;

	FFTInst.ConfigureAcc();
	FIRInst.ConfigureAcc();
	IFFTInst.ConfigureAcc();

    printf("ConfigureAcc done\n");

	// Start all accelerators - but keep them halted
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		sm_sync[ChainID*acc_offset + VALID_FLAG_OFFSET] = 0;
		sm_sync[ChainID*acc_offset + READY_FLAG_OFFSET] = 1;
		sm_sync[ChainID*acc_offset + END_FLAG_OFFSET] = 0;
	}

	sm_sync[acc_offset + FLT_VALID_FLAG_OFFSET] = 0;
	sm_sync[acc_offset + FLT_READY_FLAG_OFFSET] = 1;

	FFTInst.StartAcc();
	FIRInst.StartAcc();
	IFFTInst.StartAcc();

    printf("StartAcc done\n");
}

void FFIChain::NonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters, audio_t* m_pfScratchBufferA, unsigned CurChannel) {
	// Wait for FFT (consumer) to be ready
	while (sm_sync[ConsRdyFlag] != 1);
	sm_sync[ConsRdyFlag] = 0;
	// Write input data for FFT
	InitData(pBFSrcDst, CurChannel);
	// Inform FFT (consumer)
	sm_sync[ConsVldFlag] = 1;

	// Wait for FIR (consumer) to be ready
	while (sm_sync[FltRdyFlag] != 1);
	sm_sync[FltRdyFlag] = 0;
	// Write input data for filters
	InitFilters(pBFSrcDst, m_Filters);
	// Inform FIR (consumer)
	sm_sync[FltVldFlag] = 1;

	// Wait for IFFT (producer) to send output
	while (sm_sync[ProdVldFlag] != 1);
	sm_sync[ProdVldFlag] = 0;
	// Read back output
	ReadOutput(pBFSrcDst, m_pfScratchBufferA, CurChannel);
	// Inform IFFT (producer)
	sm_sync[ProdRdyFlag] = 1;
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

void FFIChain::InitData(CBFormat* pBFSrcDst, unsigned InitChannel) {
	int InitLength = 2 * (pBFSrcDst->m_nSamples);
	int niChannel = pBFSrcDst->m_nChannelCount - InitChannel;
	spandex_token_t CoalescedData;
	audio_t* dst;

	dst = mem + SYNC_VAR_SIZE;

	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, dst+=2)
	{
		CoalescedData.value_32_1 = pBFSrcDst->m_ppfChannels[niChannel][niSample];
		CoalescedData.value_32_2 = pBFSrcDst->m_ppfChannels[niChannel][niSample+1];

		write_mem((void *) dst, CoalescedData.value_64);
	}
}

void FFIChain::InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters) {
	int InitLength = 2 * (pBFSrcDst->m_nSamples + 1);
	spandex_token_t CoalescedData;
	audio_t* dst;

	dst = mem + (5 * acc_offset) + SYNC_VAR_SIZE;

	for (unsigned niSample = 0; niSample < InitLength; niSample++, dst+=2)
	{
		CoalescedData.value_32_1 = m_Filters[niSample].r;
		CoalescedData.value_32_2 = m_Filters[niSample].i;

		write_mem((void *) dst, CoalescedData.value_64);
	}
}

void FFIChain::ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA, unsigned ReadChannel) {
	int ReadLength =  2 * (pBFSrcDst->m_nSamples);
	int niChannel = pBFSrcDst->m_nChannelCount - ReadChannel;
	spandex_token_t CoalescedData;
	audio_t* dst;

	dst = mem + (NUM_DEVICES * acc_offset) + SYNC_VAR_SIZE;

	for (unsigned niSample = 0; niSample < ReadLength; niSample++, dst+=2)
	{
		CoalescedData.value_64 = read_mem((void *) dst);

		m_pfScratchBufferA[niSample] = CoalescedData.value_32_1;
		m_pfScratchBufferA[niSample+1] = CoalescedData.value_32_2;
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