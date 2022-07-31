#ifndef __FFT2_CFG_H__
#define __FFT2_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "_kiss_fft_guts.h"

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

typedef int32_t fft2_token_t;
typedef float fft2_native_t;

#define FFT2_FX_IL 4
#define LOGN_SAMPLES 10
#define DO_INVERSE   0
#define NUM_FFTS     1
#define DO_SHIFT     0
#define SCALE_FACTOR 0

struct fft2_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned scale_factor;
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;
	unsigned num_ffts;
	unsigned src_offset;
	unsigned dst_offset;
};

#define FFT2_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fft2_stratus_access)

struct fft2_stratus_access fft2_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.scale_factor = SCALE_FACTOR,
		.do_inverse = DO_INVERSE,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.num_ffts = NUM_FFTS,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

char fft2_dev_name[] = "fft2_stratus.0";

esp_thread_info_t fft2_thread_000[] = {
	{
		.run = true,
		.devname = fft2_dev_name,
		.ioctl_req = FFT2_STRATUS_IOC_ACCESS,
		.esp_desc = &(fft2_cfg_000[0].esp),
	}
};

#ifdef __cplusplus
}
#endif

#endif /* __FFT2_CFG_H__ */