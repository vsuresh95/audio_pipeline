#ifndef FFI_CHAIN_DATA
#define FFI_CHAIN_DATA

void FFIChain::InitData(CBFormat* pBFSrcDst, unsigned InitChannel, bool IsInit) {
	unsigned InitLength = m_nBlockSize;
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
		SrcData.value_64 = read_mem_reqv((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst, DstData.value_64);
	}
}

void FFIChain::InitFilters(CBFormat* pBFSrcDst, kiss_fft_cpx* m_Filters) {
	unsigned InitLength = 2 * m_nFFTBins;
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
		SrcData.value_64 = read_mem_reqv((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst, DstData.value_64);
	}
}

void FFIChain::ReadOutput(CBFormat* pBFSrcDst, audio_t* m_pfScratchBufferA) {
	unsigned ReadLength = m_nBlockSize + m_nOverlapLength;
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
		SrcData.value_64 = read_mem_reqodata((void *) src);

		DstData.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem((void *) dst, DstData.value_64);
	}
}

void FFIChain::InitTwiddles(CBFormat* pBFSrcDst, kiss_fft_cpx* super_twiddles) {
	unsigned InitLength = m_nBlockSize;
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
		SrcData.value_64 = read_mem_reqv((void *) src);

		DstData.value_32_1 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = FLOAT_TO_FIXED_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst, DstData.value_64);
	}
}

void FFIChain::PsychoOverlap(CBFormat* pBFSrcDst, audio_t** m_pfOverlap, unsigned CurChannel) {
	unsigned ReadLength = m_nBlockSize;
	unsigned OverlapLength = m_nOverlapLength;
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

	// First, we copy the output, scale it and account for the overlap
	// data from the previous block, for the same channel.
	for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem_reqodata((void *) src);
		OverlapData.value_64 = read_mem_reqv((void *) overlap_dst);

		DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst, DstData.value_64);
	}

	// Second, we simply copy the output (with scaling) as we are outside the overlap range.
	for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem_reqodata((void *) src);

		DstData.value_32_1 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		DstData.value_32_2 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) dst, DstData.value_64);
	}

	overlap_dst = m_pfOverlap[CurChannel];

	// Last, we copy our output (with scaling) directly to the overlap buffer only.
	// This data will be used in the first loop for the next audio block.
	for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
	{
		// Need to cast to void* for extended ASM code.
		SrcData.value_64 = read_mem_reqodata((void *) src);

		OverlapData.value_32_1 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
		OverlapData.value_32_2 = m_fFFTScaler * FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

		// Need to cast to void* for extended ASM code.
		write_mem_wtfwd((void *) overlap_dst, OverlapData.value_64);
	}
}

void FFIChain::BinaurOverlap(CBFormat* pBFSrcDst, audio_t* ppfDst, audio_t* m_pfOverlap, bool isLast, bool isFirst) {
	unsigned ReadLength =  m_nBlockSize;
	unsigned OverlapLength = m_nOverlapLength;
	device_token_t SrcData;
	device_t* src;
	audio_token_t DstData;
	audio_t* dst;
	audio_token_t OverlapData;
	audio_t* overlap_dst;

	// We coalesce 4B elements to 8B accesses for 2 reasons:
	// 1. Special Spandex forwarding cases are compatible with 8B accesses only.
	// 2. ESP NoC has a 8B interface, therefore, coalescing helps to optimize memory traffic.

	// The binauralizer is different from the psycho-acoustic filter in that
	// the output data from all channels for a single ear are summed together
	// before the overlap operation after the last channel.

	// We check if this is the last channel for that ear. If yes, we need
	// to perform overlap operation for the output.
	if (isLast)
	{
		// See init_params() for memory layout.
		src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
		dst = ppfDst;
		overlap_dst = m_pfOverlap;

		// First, we copy the output, scale it, sum it to the earlier sum,
		// and account for the overlap data from the previous block, for the same channel.
		for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem_reqodata((void *) src);
			OverlapData.value_64 = read_mem_reqv((void *) overlap_dst);
			DstData.value_64 = read_mem_reqv((void *) dst);

			DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) dst, DstData.value_64);
		}

		// Second, we simply copy the output (with scaling), sum it
		// with the previous outputs, as we are outside the overlap range.
		for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem_reqodata((void *) src);
			DstData.value_64 = read_mem_reqv((void *) dst);

			DstData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			DstData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) dst, DstData.value_64);
		}

		overlap_dst = m_pfOverlap;

		// Last, we copy our output (with scaling), sum it with the
		// previous output, directly to the overlap buffer only.
		// This data will be used in the first loop for the next audio block.
		for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem_reqodata((void *) src);
			DstData.value_64 = read_mem_reqv((void *) dst);

			OverlapData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL));
			OverlapData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL));

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) overlap_dst, OverlapData.value_64);
		}
	} else if (isFirst) {
		// See init_params() for memory layout.
		src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
		dst = ppfDst;

		// Here, we simply copy the output, sum it with the previous outputs.
		for (unsigned niSample = 0; niSample < ReadLength + OverlapLength; niSample+=2, src+=2, dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem_reqodata((void *) src);

			DstData.value_32_1 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
			DstData.value_32_2 = FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) dst, DstData.value_64);
		}
	} else {
		// See init_params() for memory layout.
		src = mem + (NUM_DEVICES * acc_len) + SYNC_VAR_SIZE;
		dst = ppfDst;

		// Here, we simply copy the output, sum it with the previous outputs.
		for (unsigned niSample = 0; niSample < ReadLength + OverlapLength; niSample+=2, src+=2, dst+=2)
		{
			// Need to cast to void* for extended ASM code.
			SrcData.value_64 = read_mem_reqodata((void *) src);
			DstData.value_64 = read_mem_reqv((void *) dst);

			DstData.value_32_1 = DstData.value_32_1 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_1, AUDIO_FX_IL);
			DstData.value_32_2 = DstData.value_32_2 + FIXED_TO_FLOAT_WRAP(SrcData.value_32_2, AUDIO_FX_IL);

			// Need to cast to void* for extended ASM code.
			write_mem_wtfwd((void *) dst, DstData.value_64);
		}
	}
}

void FFIChain::UpdateSync(unsigned FlagOFfset, int64_t UpdateValue) {
	volatile device_t* sync = mem + FlagOFfset;

	asm volatile ("fence w, w");

	// Need to cast to void* for extended ASM code.
	write_mem_wtfwd((void *) sync, UpdateValue);

	asm volatile ("fence w, w");
}

void FFIChain::SpinSync(unsigned FlagOFfset, int64_t SpinValue) {
	volatile device_t* sync = mem + FlagOFfset;
	int64_t ExpectedValue = SpinValue;
	int64_t ActualValue = 0xcafedead;

	while (ActualValue != ExpectedValue) {
		// Need to cast to void* for extended ASM code.
		ActualValue = read_mem_reqodata((void *) sync);
	}
}

bool FFIChain::TestSync(unsigned FlagOFfset, int64_t TestValue) {
	volatile device_t* sync = mem + FlagOFfset;
	int64_t ExpectedValue = TestValue;
	int64_t ActualValue = 0xcafedead;

	// Need to cast to void* for extended ASM code.
	ActualValue = read_mem_reqodata((void *) sync);

	if (ActualValue != ExpectedValue) return false;
	else return true;
}

#endif // FFI_CHAIN_DATA
