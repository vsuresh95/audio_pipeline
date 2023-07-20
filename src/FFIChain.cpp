#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <FFIChain.hpp>
#include <FFIChainHelper.hpp>
#include <FFIChainData.hpp>
#include <FFIChainDMA.hpp>
#include <FFIChainMono.hpp>

#if (USE_MONOLITHIC_ACC == 0)
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
	FFTInst.inverse = 0;
	IFFTInst.inverse = 1;

	InitSyncFlags();
	
	// After assigning the parameters, we call ConfigureAcc() for
	// each accelerator object to program accelerator register
	// with these parameters.
	FFTInst.ConfigureAcc();
	FIRInst.ConfigureAcc();
	IFFTInst.ConfigureAcc();

	if (DO_ROTATE_OFFLOAD) {
		RotateInst.ProbeAcc(0);

		RotateInst.ptable = ptable;
		RotateInst.mem_size = mem_size;
		RotateInst.acc_size = acc_size;
		RotateInst.SpandexReg = SpandexConfig.spandex_reg;
		RotateInst.CoherenceMode = CoherenceMode;
		RotateInst.m_nChannelCount = m_nChannelCount;

		RotateInst.ConfigureAcc();
	}

	// Reset all sync variables to default values.
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		UpdateSync(ChainID*acc_len + VALID_FLAG_OFFSET, 0);
		UpdateSync(ChainID*acc_len + READY_FLAG_OFFSET, 1);
		UpdateSync(ChainID*acc_len + END_FLAG_OFFSET, 0);
	}

	UpdateSync(acc_len + FLT_VALID_FLAG_OFFSET, 0);
	UpdateSync(acc_len + FLT_READY_FLAG_OFFSET, 1);
}

void FFIChain::StartAcc() {
	// We have separeted ConfigureAcc() from StartAcc() in case we
	// don't use shared memory chaining.
	FFTInst.StartAcc();
	FIRInst.StartAcc();
	IFFTInst.StartAcc();
}

void FFIChain::InitSyncFlags() {
	FFTInst.prod_valid_offset = VALID_FLAG_OFFSET;
	FFTInst.prod_ready_offset = READY_FLAG_OFFSET;
	FFTInst.cons_valid_offset = acc_len + VALID_FLAG_OFFSET;
	FFTInst.cons_ready_offset = acc_len + READY_FLAG_OFFSET;
	FFTInst.input_offset = SYNC_VAR_SIZE;
	FFTInst.output_offset = acc_len + SYNC_VAR_SIZE;

	FIRInst.prod_valid_offset = acc_len + VALID_FLAG_OFFSET;
	FIRInst.prod_ready_offset = acc_len + READY_FLAG_OFFSET;
	FIRInst.flt_prod_valid_offset = acc_len + FLT_VALID_FLAG_OFFSET;
	FIRInst.flt_prod_ready_offset = acc_len + FLT_READY_FLAG_OFFSET;
	FIRInst.cons_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	FIRInst.cons_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	FIRInst.input_offset = acc_len + SYNC_VAR_SIZE;
	FIRInst.flt_input_offset = 5 * acc_len;
	FIRInst.twd_input_offset = 7 * acc_len;
	FIRInst.output_offset = (2 * acc_len) + SYNC_VAR_SIZE;

	IFFTInst.prod_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	IFFTInst.prod_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	IFFTInst.cons_valid_offset = (3 * acc_len) + VALID_FLAG_OFFSET;
	IFFTInst.cons_ready_offset = (3 * acc_len) + READY_FLAG_OFFSET;
	IFFTInst.input_offset = (2 * acc_len) + SYNC_VAR_SIZE;
	IFFTInst.output_offset = (3 * acc_len) + SYNC_VAR_SIZE;
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

		// Start and check for termination of each accelerator.
		StartCounter();
		FFTInst.StartAcc();
		FFTInst.TerminateAcc();
		FIRInst.StartAcc();
		FIRInst.TerminateAcc();
		IFFTInst.StartAcc();
		IFFTInst.TerminateAcc();
		EndCounter(2);

		WriteScratchReg(0x8);
		StartCounter();
		// Read back output from IFFT
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
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
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
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
			FFTInst.StartAcc();
			FFTInst.TerminateAcc();
			FIRInst.StartAcc();
			FIRInst.TerminateAcc();
			IFFTInst.StartAcc();
			IFFTInst.TerminateAcc();
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
#endif

void FFIChain::PsychoNonPipelineProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned ChannelsLeft = m_nChannelCount;

	while (ChannelsLeft != 0) {
		WriteScratchReg(0x2);
		StartCounter();
		// Wait for FFT (consumer) to be ready.
		SpinSync(ConsRdyFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ConsRdyFlag, 0);
		// Write input data for FFT.
		InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
		EndCounter(0);
		WriteScratchReg(0);

		unsigned iChannelOrder = int(sqrt(m_nChannelCount - ChannelsLeft));

		WriteScratchReg(0x4);
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
		WriteScratchReg(0);

		StartCounter();
		// Wait for IFFT (producer) to send output.
		SpinSync(ProdVldFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ProdVldFlag, 0);
		EndCounter(2);

		WriteScratchReg(0x8);
		StartCounter();
		// Read back output from IFFT
		PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - ChannelsLeft);
		// Inform IFFT (producer) - ready for next iteration.
		UpdateSync(ProdRdyFlag, 1);
		EndCounter(3);	
		WriteScratchReg(0);

		ChannelsLeft--;
	}
}

void FFIChain::BinaurNonPipelineProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
    for(unsigned niEar = 0; niEar < 2; niEar++) {
		unsigned ChannelsLeft = m_nChannelCount;

		while (ChannelsLeft != 0) {
			WriteScratchReg(0x100);
			StartCounter();
			// Wait for FFT (consumer) to be ready.
			SpinSync(ConsRdyFlag, 1);
			// Reset flag for next iteration.
			UpdateSync(ConsRdyFlag, 0);
			// Write input data for FFT.
			InitData(pBFSrcDst, m_nChannelCount - ChannelsLeft, true);
			EndCounter(4);
			WriteScratchReg(0);

			WriteScratchReg(0x200);
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
			WriteScratchReg(0);

			StartCounter();
			// Wait for IFFT (producer) to send output.
			SpinSync(ProdVldFlag, 1);
			// Reset flag for next iteration.
			UpdateSync(ProdVldFlag, 0);
			EndCounter(6);

			WriteScratchReg(0x400);
			StartCounter();
			// Read back output from IFFT
			BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (ChannelsLeft == 1), (ChannelsLeft == m_nChannelCount));
			// Inform IFFT (producer) - ready for next iteration.
			UpdateSync(ProdRdyFlag, 1);
			EndCounter(7);	
			WriteScratchReg(0);

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
				WriteScratchReg(0x2);
        		StartCounter();
				InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, true);
        		EndCounter(0);
				WriteScratchReg(0);
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
				WriteScratchReg(0x4);
        		StartCounter();
				InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
        		EndCounter(1);
				WriteScratchReg(0);
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
				WriteScratchReg(0x8);
        		StartCounter();
				PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - OutputChannelsLeft);
        		EndCounter(2);
				WriteScratchReg(0);
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
					WriteScratchReg(0x100);
        			StartCounter();
					InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, false);
        			EndCounter(3);
					WriteScratchReg(0);
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
					WriteScratchReg(0x200);
        			StartCounter();
					InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - FilterChannelsLeft]);
        			EndCounter(4);
					WriteScratchReg(0);
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
					WriteScratchReg(0x400);
        			StartCounter();
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1), (OutputChannelsLeft == m_nChannelCount));
        			EndCounter(5);
					WriteScratchReg(0);
					// Inform IFFT (producer)
					UpdateSync(ProdRdyFlag, 1);
					OutputChannelsLeft--;
				}
			}
		}
	}
}

void FFIChain::OffloadRotateOrder(CBFormat* pBFSrcDst) {
	// Write input data for FFT.
	unsigned InitLength = m_nBlockSize;
	unsigned InitChannel = m_nChannelCount;
	audio_token_t SrcData_i;
	audio_t* src_i;
	device_token_t DstData_i;
	device_t* dst_i;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for(unsigned niChannel = 0; niChannel < InitChannel; niChannel++)
	{
		// See init_params() for memory layout.
		src_i = pBFSrcDst->m_ppfChannels[niChannel];
		dst_i = mem + 10 * acc_len;

		for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src_i+=2, dst_i+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData_i.value_64 = read_mem_reqv((void *) src_i);

			DstData_i.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_1, ROTATE_FX_IL);
			DstData_i.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData_i.value_32_2, ROTATE_FX_IL);

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) dst_i, DstData_i.value_64);
		}
	}
	EndCounter(0);

	// Start and check for termination of each accelerator.
	StartCounter();
	RotateInst.StartAcc();
	RotateInst.TerminateAcc();
	EndCounter(1);

	device_token_t SrcData_o;
	device_t* src_o;
	audio_token_t DstData_o;
	audio_t* dst_o;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	StartCounter();
	for(unsigned niChannel = 0; niChannel < InitChannel; niChannel++)
	{
		// See init_params() for memory layout.
		src_o = mem + 10 * acc_len + m_nChannelCount * num_samples;
		dst_o = pBFSrcDst->m_ppfChannels[niChannel];

		for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src_o+=2, dst_o+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData_o.value_64 = read_mem_reqodata((void *) src_o);

			DstData_o.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_1, ROTATE_FX_IL);
			DstData_o.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData_o.value_32_2, ROTATE_FX_IL);

			// Need to cast to void* for extended ASM code.
			write_mem((void *) dst_o, DstData_o.value_64);
		}
	}
	EndCounter(2);
}

void FFIChain::PrintTimeInfo(unsigned factor, bool isPsycho) {
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD) {
		if (isPsycho) {
			printf("Psycho Init Data\t = %llu\n", TotalTime[0]/factor);
			printf("Psycho Init Filters\t = %llu\n", TotalTime[1]/factor);
			printf("Psycho Acc execution\t = %llu\n", TotalTime[2]/factor);
			printf("Psycho Output Read\t = %llu\n", TotalTime[3]/factor);
		} else {
			printf("Binaur Init Data\t = %llu\n", TotalTime[4]/factor);
			printf("Binaur Init Filters\t = %llu\n", TotalTime[5]/factor);
			printf("Binaur Acc execution\t = %llu\n", TotalTime[6]/factor);
			printf("Binaur Output Read\t = %llu\n", TotalTime[7]/factor);
		}
	} else if (DO_PP_CHAIN_OFFLOAD) {
		if (isPsycho) {
			printf("Psycho Init Data\t = %llu\n", TotalTime[0]/factor);
			printf("Psycho Init Filters\t = %llu\n", TotalTime[1]/factor);
			printf("Psycho Acc execution\t = 0\n");
			printf("Psycho Output Read\t = %llu\n", TotalTime[2]/factor);
		} else {
			printf("Binaur Init Data\t = %llu\n", TotalTime[3]/factor);
			printf("Binaur Init Filters\t = %llu\n", TotalTime[4]/factor);
			printf("Binaur Acc execution\t = 0\n");
			printf("Binaur Output Read\t = %llu\n", TotalTime[5]/factor);
		}
	}
}
