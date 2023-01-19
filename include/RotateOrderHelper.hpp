#ifndef ROTATE_ORDER_HELPER
#define ROTATE_ORDER_HELPER

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

// Helper function to perform memory write using Spandex request types.
// By default, the writes are regular stores.
// See CohDefines.hpp for definition of WRITE_CODE.
static inline void write_mem_opt (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE_OPT)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

// Helper function to perform memory read using Spandex request types.
// By default, the reads are regular loads.
// See CohDefines.hpp for definition of READ_CODE.
static inline int64_t read_mem_opt (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE_OPT) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

#endif // ROTATE_ORDER_HELPER