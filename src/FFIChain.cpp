#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <FFIChain.hpp>
#include <FFIChainHelper.hpp>
#include <FFIChainData.hpp>

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
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
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
	ReadOutput(pBFSrcDst, m_pfScratchBufferA);
	// Inform IFFT (producer) - ready for next iteration.
	sm_sync[ProdRdyFlag] = 1;
}

void FFIChain::PsychoProcess(CBFormat* pBFSrcDst, kiss_fft_cpx** m_Filters, audio_t** m_pfOverlap) {
	unsigned InputChannelsLeft = m_nChannelCount;
	unsigned FilterChannelsLeft = m_nChannelCount;
	unsigned OutputChannelsLeft = m_nChannelCount;

	while (InputChannelsLeft != 0 || FilterChannelsLeft != 0 || OutputChannelsLeft != 0) {
		if (InputChannelsLeft) {
			// Wait for FFT (consumer) to be ready
			if (sm_sync[ConsRdyFlag] == 1) {
				sm_sync[ConsRdyFlag] = 0;
				// Write input data for FFT
				InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft);
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
				InitFilters(pBFSrcDst, m_Filters[iChannelOrder]);
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
				PsychoOverlap(pBFSrcDst, m_pfOverlap, m_nChannelCount - OutputChannelsLeft);
				// Inform IFFT (producer)
				sm_sync[ProdRdyFlag] = 1;
				OutputChannelsLeft--;
			}
		}
	}
}

void FFIChain::BinaurProcess(CBFormat* pBFSrcDst, audio_t** ppfDst, kiss_fft_cpx*** m_Filters, audio_t** m_pfOverlap) {
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
					InitData(pBFSrcDst, m_nChannelCount - InputChannelsLeft);
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
					InitFilters(pBFSrcDst, m_Filters[niEar][m_nChannelCount - FilterChannelsLeft]);
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
					BinaurOverlap(pBFSrcDst, ppfDst[niEar], m_pfOverlap[niEar], (OutputChannelsLeft == 1));
					// Inform IFFT (producer)
					sm_sync[ProdRdyFlag] = 1;
					OutputChannelsLeft--;
				}
			}
		}
	}
}