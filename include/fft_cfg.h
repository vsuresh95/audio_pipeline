#ifndef __FFT_CFG_H__
#define __FFT_CFG_H__

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

typedef int fft_token_t;
typedef float fft_native_t;

#define FFT_FX_IL 8
#define NUM_BATCHES 1
#define LOG_LEN 10
#define DO_BITREV 1

struct fft_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned batch_size;
	unsigned log_len;
	unsigned do_bitrev;
	unsigned src_offset;
	unsigned dst_offset;
};

#define FFT_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fft_stratus_access)

struct fft_stratus_access fft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.batch_size = NUM_BATCHES,
		.log_len = LOG_LEN,
		.do_bitrev = DO_BITREV,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

char fft_dev_name[] = "fft_stratus.0";

esp_thread_info_t fft_thread_000[] = {
	{
		.run = true,
		.devname = fft_dev_name,
		.ioctl_req = FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(fft_cfg_000[0].esp),
	}
};

#ifdef __cplusplus
}
#endif

#endif /* __FFT_CFG_H__ */