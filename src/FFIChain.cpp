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
	FFTInst.StartAcc();
	FFTInst.TerminateAcc();
	FIRInst.StartAcc();
	FIRInst.TerminateAcc();
	IFFTInst.StartAcc();
	IFFTInst.TerminateAcc();
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

    StartCounter();
	// Read back output from IFFT
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
				WriteScratchReg(0x2);
        		StartCounter();
				InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft, true);
    			asm volatile ("fence w, w");
        		EndCounter(0);
				WriteScratchReg(0);
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
				WriteScratchReg(0x4);
        		StartCounter();
				InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
    			asm volatile ("fence w, w");
        		EndCounter(1);
				WriteScratchReg(0);
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
				if (sm_sync[FltRdyFlag] == 1) {
					sm_sync[FltRdyFlag] = 0;
					// Write input data for filters
					WriteScratchReg(0x200);
        			StartCounter();
					InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - FilterChannelsLeft]);
    				asm volatile ("fence w, w");
        			EndCounter(4);
					WriteScratchReg(0);
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
					WriteScratchReg(0x400);
        			StartCounter();
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1));
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

void FFIChain::PrintTimeInfo(unsigned factor, bool isPsycho) {
    printf("---------------------------------------------\n");
    printf("TOTAL TIME FROM %s\n", Name);
    printf("---------------------------------------------\n");
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD) {
		printf("Init Data\t = %lu\n", TotalTime[0]/factor);
		printf("Init Filters\t = %lu\n", TotalTime[1]/factor);
		printf("Acc execution\t = %lu\n", TotalTime[2]/factor);
		printf("Output Read\t = %lu\n", TotalTime[3]/factor);
	} else if (DO_PP_CHAIN_OFFLOAD) {
        if (isPsycho) {
                printf("Psycho Init Data\t = %lu\n", TotalTime[0]/factor);
                printf("Psycho Init Filters\t = %lu\n", TotalTime[1]/factor);
                printf("Psycho Output Read\t = %lu\n", TotalTime[2]/factor);
        } else {
                printf("Binaur Init Data\t = %lu\n", TotalTime[3]/factor);
                printf("Binaur Init Filters\t = %lu\n", TotalTime[4]/factor);
                printf("Binaur Output Read\t = %lu\n", TotalTime[5]/factor);
        }
	}
}