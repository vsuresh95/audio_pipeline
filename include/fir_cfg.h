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

#define FIR_FX_IL 4
#define DATA_LENGTH 1024

struct fir_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned data_length;
    unsigned src_offset;
	unsigned dst_offset;
};

#define FIR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fir_stratus_access)

struct fir_stratus_access fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.data_length = DATA_LENGTH,
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
