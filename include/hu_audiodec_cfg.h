#ifndef __HU_AUDIODEC_CFG_H__
#define __HU_AUDIODEC_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libesp.h>
#include <esp.h>
#include <esp_accelerator.h>

#ifdef __cplusplus
}
#endif

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

// #include "hu_audiodec_rtl.h"

typedef int32_t token_t;

/* <<--params-->> */
#define CFG_REGS_0  0   // 0: dummy
#define CFG_REGS_1  0   // 1: dummy, channel is fixed a 16
#define CFG_REGS_2  8   // audio block size
#define CFG_REGS_3  0   // dma_read_index  
#define CFG_REGS_4  64  // dma_write_index 128*32b/64b = 64 
#define CFG_REGS_5  0   // 5-07: dummy
#define CFG_REGS_6  0
#define CFG_REGS_7  0   
#define CFG_REGS_8  39413   // 08: cfg_cos_alpha;  
#define CFG_REGS_9  60968   // 09: cfg_sin_alpha;
#define CFG_REGS_10 -46013   // 10: cfg_cos_beta;
#define CFG_REGS_11 -56152   // 11: cfg_sin_beta;
#define CFG_REGS_12 -35750   // 12: cfg_cos_gamma;
#define CFG_REGS_13 -22125   // 13: cfg_sin_gamma;
#define CFG_REGS_14 -39414   // 14: cfg_cos_2_alpha;
#define CFG_REGS_15 57035   // 15: cfg_sin_2_alpha;
#define CFG_REGS_16 -15211   // 16: cfg_cos_2_beta;
#define CFG_REGS_17 10276   // 17: cfg_sin_2_beta;
#define CFG_REGS_18 27688   // 18: cfg_cos_2_gamma;
#define CFG_REGS_19 -48707   // 19: cfg_sin_2_gamma;
#define CFG_REGS_20 -42691   // 20: cfg_cos_3_alpha;
#define CFG_REGS_21 -11292   // 21: cfg_sin_3_alpha;
#define CFG_REGS_22 48018   // 22: cfg_cos_3_beta;
#define CFG_REGS_23 23121   // 23: cfg_sin_3_beta;
#define CFG_REGS_24 -53190   // 24: cfg_cos_3_gamma;
#define CFG_REGS_25 22878   // 25: cfg_sin_3_gamma;
#define CFG_REGS_26 0		// 26-31: dummy
#define CFG_REGS_27 0
#define CFG_REGS_28 0
#define CFG_REGS_29 0
#define CFG_REGS_30 0
#define CFG_REGS_31 0

struct hu_audiodec_rtl_access {
	struct esp_access esp;
	/* <<--regs-->> */
	int32_t cfg_regs_31;
	int32_t cfg_regs_30;
	int32_t cfg_regs_26;
	int32_t cfg_regs_27;
	int32_t cfg_regs_24;
	int32_t cfg_regs_25;
	int32_t cfg_regs_22;
	int32_t cfg_regs_23;
	int32_t cfg_regs_8;
	int32_t cfg_regs_20;
	int32_t cfg_regs_9;
	int32_t cfg_regs_21;
	int32_t cfg_regs_6;
	int32_t cfg_regs_7;
	int32_t cfg_regs_4;
	int32_t cfg_regs_5;
	int32_t cfg_regs_2;
	int32_t cfg_regs_3;
	int32_t cfg_regs_0;
	int32_t cfg_regs_28;
	int32_t cfg_regs_1;
	int32_t cfg_regs_29;
	int32_t cfg_regs_19;
	int32_t cfg_regs_18;
	int32_t cfg_regs_17;
	int32_t cfg_regs_16;
	int32_t cfg_regs_15;
	int32_t cfg_regs_14;
	int32_t cfg_regs_13;
	int32_t cfg_regs_12;
	int32_t cfg_regs_11;
	int32_t cfg_regs_10;
	int32_t src_offset;
	int32_t dst_offset;
};

#define HU_AUDIODEC_RTL_IOC_ACCESS	_IOW ('S', 0, struct hu_audiodec_rtl_access)

struct hu_audiodec_rtl_access hu_audiodec_cfg_000[] = {
    {
		/* <<--descriptor-->> */
		.cfg_regs_31 = CFG_REGS_31,
		.cfg_regs_30 = CFG_REGS_30,
		.cfg_regs_26 = CFG_REGS_26,
		.cfg_regs_27 = CFG_REGS_27,
		.cfg_regs_24 = CFG_REGS_24,
		.cfg_regs_25 = CFG_REGS_25,
		.cfg_regs_22 = CFG_REGS_22,
		.cfg_regs_23 = CFG_REGS_23,
		.cfg_regs_8 = CFG_REGS_8,
		.cfg_regs_20 = CFG_REGS_20,
		.cfg_regs_9 = CFG_REGS_9,
		.cfg_regs_21 = CFG_REGS_21,
		.cfg_regs_6 = CFG_REGS_6,
		.cfg_regs_7 = CFG_REGS_7,
		.cfg_regs_4 = CFG_REGS_4,
		.cfg_regs_5 = CFG_REGS_5,
		.cfg_regs_2 = CFG_REGS_2,
		.cfg_regs_3 = CFG_REGS_3,
		.cfg_regs_0 = CFG_REGS_0,
		.cfg_regs_28 = CFG_REGS_28,
		.cfg_regs_1 = CFG_REGS_1,
		.cfg_regs_29 = CFG_REGS_29,
		.cfg_regs_19 = CFG_REGS_19,
		.cfg_regs_18 = CFG_REGS_18,
		.cfg_regs_17 = CFG_REGS_17,
		.cfg_regs_16 = CFG_REGS_16,
		.cfg_regs_15 = CFG_REGS_15,
		.cfg_regs_14 = CFG_REGS_14,
		.cfg_regs_13 = CFG_REGS_13,
		.cfg_regs_12 = CFG_REGS_12,
		.cfg_regs_11 = CFG_REGS_11,
		.cfg_regs_10 = CFG_REGS_10,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "hu_audiodec_rtl.0",
		.ioctl_req = HU_AUDIODEC_RTL_IOC_ACCESS,
		.esp_desc = &(hu_audiodec_cfg_000[0].esp),
	}
};

#endif /* __HU_AUDIODEC_CFG_H__ */