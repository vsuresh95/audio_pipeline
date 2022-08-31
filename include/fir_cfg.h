#ifndef __FIR_CFG_H__
#define __FIR_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libesp.h>
#include <esp.h>
#include <esp_accelerator.h>

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

typedef int32_t fir_token_t;
typedef float fir_native_t;

#define FIR_FX_IL 4
#define LOGN_SAMPLES 10
#define DO_INVERSE   0
#define NUM_FIRS     1
#define DO_SHIFT     0
#define SCALE_FACTOR 0

struct fir_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned scale_factor;
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;
	unsigned num_firs;
	unsigned src_offset;
	unsigned dst_offset;
};

#define FIR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fir_stratus_access)

struct fir_stratus_access fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.scale_factor = SCALE_FACTOR,
		.do_inverse = DO_INVERSE,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.num_firs = NUM_FIRS,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

char fir_dev_name[] = "fir_stratus.0";

esp_thread_info_t fir_thread_000[] = {
	{
		.run = true,
		.devname = fir_dev_name,
		.ioctl_req = FIR_STRATUS_IOC_ACCESS,
		.esp_desc = &(fir_cfg_000[0].esp),
	}
};

#ifdef __cplusplus
}
#endif

#endif /* __FIR_CFG_H__ */