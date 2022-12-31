#ifndef FFI_CHAIN_DATA
#define FFI_CHAIN_DATA

void FFIChain::InitData(CBFormat* pBFSrcDst, unsigned InitChannel) {
	int InitLength = m_nBlockSize;
	audio_token_t SrcData;
	audio_t* src;
	device_token_t DstData;
	device_t* dst;

	// See init_params() for memory layout.
	src = pBFSrcDst->m_ppfChannels[InitChannel];
	dst = mem + SYNC_VAR_SIZE;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}
}

void FFIChain::InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters) {
	int InitLength = 2 * m_nFFTBins;
	audio_token_t SrcData;
	audio_t* src;
	device_token_t DstData;
	device_t* dst;

	// See init_params() for memory layout.
	src = (audio_t *) m_Filters;
	dst = mem + (5 * acc_len);

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}
}

void FFIChain::ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA) {
	int ReadLength = m_nBlockSize + m_nOverlapLength;
	device_token_t SrcData;
	device_t* src;
	audio_token_t DstData;
	audio_t* dst;

	// See init_params() for memory layout.
	src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
	dst = m_pfScratchBufferA;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		DstData.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}
}

void FFIChain::InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles) {
	int InitLength = m_nBlockSize;
	audio_token_t SrcData;
	audio_t* src;
	device_token_t DstData;
	device_t* dst;

	// See init_params() for memory layout.
	src = (audio_t *) super_twiddles;
	dst = mem + (7 * acc_len);

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}
}

void FFIChain::PsychoOverlap(CBFormat* pBFSrcDst, audio_t** m_pfOverlap, unsigned CurChannel) {
	int ReadLength = m_nBlockSize;
	int OverlapLength = m_nOverlapLength;
	device_token_t SrcData;
	device_t* src;
	audio_token_t DstData;
	audio_t* dst;
	audio_token_t OverlapData;
	audio_t* overlap_dst;

	// See init_params() for memory layout.
	src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
	dst = pBFSrcDst->m_ppfChannels[CurChannel];
	overlap_dst = m_pfOverlap[CurChannel];

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);
		OverlapData.value_64 = read_mem((void *) overlap_dst);

		DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}

	for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		DstData.value_32_1 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}

	overlap_dst = m_pfOverlap[CurChannel];

	for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem((void *) src);

		OverlapData.value_32_1 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		OverlapData.value_32_2 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) overlap_dst, OverlapData.value_64);
	}
}

void FFIChain::BinaurOverlap(CBFormat* pBFSrcDst, audio_t* ppfDst, audio_t* m_pfOverlap, bool isLast) {
	int ReadLength =  m_nBlockSize;
	int OverlapLength = m_nOverlapLength;
	device_token_t SrcData;
	device_t* src;
	audio_token_t DstData;
	audio_t* dst;
	audio_token_t OverlapData;
	audio_t* overlap_dst;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.
	if (isLast)
	{
		// See init_params() for memory layout.
		src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
		dst = ppfDst;
		overlap_dst = m_pfOverlap;

		for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem((void *) src);
			OverlapData.value_64 = read_mem((void *) overlap_dst);
			DstData.value_64 = read_mem((void *) dst);

			DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem((void *) dst, DstData.value_64);
		}

		for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem((void *) src);
			DstData.value_64 = read_mem((void *) dst);

			DstData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			DstData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem((void *) dst, DstData.value_64);
		}

		overlap_dst = m_pfOverlap;

		for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem((void *) src);
			DstData.value_64 = read_mem((void *) dst);

			OverlapData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			OverlapData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem((void *) overlap_dst, OverlapData.value_64);
		}
	} else {
		// See init_params() for memory layout.
		src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
		dst = ppfDst;

		for (unsigned niSample = 0; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem((void *) src);
			DstData.value_64 = read_mem((void *) dst);

			DstData.value_32_1 = DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
			DstData.value_32_2 = DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

			// Need to cast to void* for extended ASM code.
			write_mem((void *) dst, DstData.value_64);
		}
	}
}

#endif // FFI_CHAIN_DATA