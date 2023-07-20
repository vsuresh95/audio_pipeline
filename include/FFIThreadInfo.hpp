#ifndef FFI_THREAD_INFO_H
#define FFI_THREAD_INFO_H

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

#define AUDIO_FFT_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_fft_stratus_access)
#define AUDIO_FIR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_fir_stratus_access)
#define AUDIO_DMA_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_dma_stratus_access)
#define AUDIO_FFI_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_ffi_stratus_access)
#define EPOCHS_FFT_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct epochs_fft_stratus_access)
#define HU_AUDIODEC_RTL_IOC_ACCESS		_IOW ('S', 0, struct hu_audiodec_rtl_access)

char fft_dev_name[] = "audio_fft_stratus.0";
char ifft_dev_name[] = "audio_fft_stratus.1";
char fir_dev_name[] = "audio_fir_stratus.0";
char dma_dev_name[] = "audio_dma_stratus.0";
char ffi_dev_name[] = "audio_ffi_stratus.0";

char epochs_fft_dev_name[] = "fft2_stratus.0";
char epochs_ifft_dev_name[] = "fft2_stratus.1";
char rotate_dev_name[] = "hu_audiodec_rtl.0";

/* <<--params-def-->> */
#define LOGN_SAMPLES 10
#define DO_SHIFT 0
#define NUM_FFTS     1
#define SCALE_FACTOR 1
#define START_OFFSET 1

/* <<--params-def-->> */
#define CFG_REGS_24 1
#define CFG_REGS_25 1
#define CFG_REGS_22 1
#define CFG_REGS_23 1
#define CFG_REGS_8 1
#define CFG_REGS_20 1
#define CFG_REGS_9 1
#define CFG_REGS_21 1
#define CFG_REGS_4 1
#define CFG_REGS_2 1
#define CFG_REGS_3 1
#define CFG_REGS_19 1
#define CFG_REGS_18 1
#define CFG_REGS_17 1
#define CFG_REGS_16 1
#define CFG_REGS_15 1
#define CFG_REGS_14 1
#define CFG_REGS_13 1
#define CFG_REGS_12 1
#define CFG_REGS_11 1
#define CFG_REGS_10 1

struct audio_fft_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;

	// ASI sync flag offsets
    unsigned prod_valid_offset;
    unsigned prod_ready_offset;
    unsigned cons_valid_offset;
    unsigned cons_ready_offset;
    unsigned input_offset;
    unsigned output_offset;

	unsigned src_offset;
	unsigned dst_offset;
	unsigned spandex_conf;
};

struct audio_fir_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;

	// ASI sync flag offsets
    unsigned prod_valid_offset;
    unsigned prod_ready_offset;
    unsigned flt_prod_valid_offset;
    unsigned flt_prod_ready_offset;
    unsigned cons_valid_offset;
    unsigned cons_ready_offset;
    unsigned input_offset;
    unsigned flt_input_offset;
    unsigned twd_input_offset;
    unsigned output_offset;

	unsigned src_offset;
	unsigned dst_offset;
	unsigned spandex_conf;
};

struct audio_dma_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned start_offset;
	unsigned src_offset;
	unsigned dst_offset;
	unsigned spandex_conf;
};

struct audio_ffi_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;

	// ASI sync flag offsets
    unsigned prod_valid_offset;
    unsigned prod_ready_offset;
    unsigned flt_prod_valid_offset;
    unsigned flt_prod_ready_offset;
    unsigned cons_valid_offset;
    unsigned cons_ready_offset;
    unsigned input_offset;
    unsigned flt_input_offset;
    unsigned twd_input_offset;
    unsigned output_offset;

	unsigned src_offset;
	unsigned dst_offset;
	unsigned spandex_conf;
};

struct epochs_fft_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned scale_factor;
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;
	unsigned num_ffts;

	unsigned src_offset;
	unsigned dst_offset;
	unsigned spandex_conf;
};

struct hu_audiodec_rtl_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned cfg_regs_31;
	unsigned cfg_regs_30;
	unsigned cfg_regs_26;
	unsigned cfg_regs_27;
	unsigned cfg_regs_24;
	unsigned cfg_regs_25;
	unsigned cfg_regs_22;
	unsigned cfg_regs_23;
	unsigned cfg_regs_8;
	unsigned cfg_regs_20;
	unsigned cfg_regs_9;
	unsigned cfg_regs_21;
	unsigned cfg_regs_6;
	unsigned cfg_regs_7;
	unsigned cfg_regs_4;
	unsigned cfg_regs_5;
	unsigned cfg_regs_2;
	unsigned cfg_regs_3;
	unsigned cfg_regs_0;
	unsigned cfg_regs_28;
	unsigned cfg_regs_1;
	unsigned cfg_regs_29;
	unsigned cfg_regs_19;
	unsigned cfg_regs_18;
	unsigned cfg_regs_17;
	unsigned cfg_regs_16;
	unsigned cfg_regs_15;
	unsigned cfg_regs_14;
	unsigned cfg_regs_13;
	unsigned cfg_regs_12;
	unsigned cfg_regs_11;
	unsigned cfg_regs_10;
	unsigned src_offset;
	unsigned dst_offset;
};

struct audio_fft_stratus_access audio_fft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = 0,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,

		.prod_valid_offset = 0,
		.prod_ready_offset = 0,
		.cons_valid_offset = 0,
		.cons_ready_offset = 0,
		.input_offset = 0,
		.output_offset = 0,
		
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct audio_fft_stratus_access audio_ifft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = 1,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,

		.prod_valid_offset = 0,
		.prod_ready_offset = 0,
		.cons_valid_offset = 0,
		.cons_ready_offset = 0,
		.input_offset = 0,
		.output_offset = 0,
		
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct audio_fir_stratus_access audio_fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = 0,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,

    	.prod_valid_offset = 0,
    	.prod_ready_offset = 0,
    	.flt_prod_valid_offset = 0,
    	.flt_prod_ready_offset = 0,
    	.cons_valid_offset = 0,
    	.cons_ready_offset = 0,
    	.input_offset = 0,
    	.flt_input_offset = 0,
    	.twd_input_offset = 0,
    	.output_offset = 0,
		
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct audio_dma_stratus_access audio_dma_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.start_offset = START_OFFSET,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct audio_ffi_stratus_access audio_ffi_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = 0,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,

    	.prod_valid_offset = 0,
    	.prod_ready_offset = 0,
    	.flt_prod_valid_offset = 0,
    	.flt_prod_ready_offset = 0,
    	.cons_valid_offset = 0,
    	.cons_ready_offset = 0,
    	.input_offset = 0,
    	.flt_input_offset = 0,
    	.twd_input_offset = 0,
    	.output_offset = 0,
		
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct epochs_fft_stratus_access epochs_fft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.scale_factor = SCALE_FACTOR,	
		.do_inverse = 0,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.num_ffts = NUM_FFTS,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct epochs_fft_stratus_access epochs_ifft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.scale_factor = SCALE_FACTOR,	
		.do_inverse = 1,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.num_ffts = NUM_FFTS,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

struct hu_audiodec_rtl_access hu_audiodec_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.cfg_regs_24 = CFG_REGS_24,
		.cfg_regs_25 = CFG_REGS_25,
		.cfg_regs_22 = CFG_REGS_22,
		.cfg_regs_23 = CFG_REGS_23,
		.cfg_regs_8 = CFG_REGS_8,
		.cfg_regs_20 = CFG_REGS_20,
		.cfg_regs_9 = CFG_REGS_9,
		.cfg_regs_21 = CFG_REGS_21,
		.cfg_regs_4 = 0,
		.cfg_regs_2 = 8,
		.cfg_regs_3 = 0,
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

#if (EPOCHS_TARGET == 0)
esp_thread_info_t fft_cfg_000[] = {
	{
		.run = true,
		.devname = fft_dev_name,
		.ioctl_req = AUDIO_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_fft_cfg_000[0].esp),
	}
};

esp_thread_info_t ifft_cfg_000[] = {
	{
		.run = true,
		.devname = ifft_dev_name,
		.ioctl_req = AUDIO_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_ifft_cfg_000[0].esp),
	}
};
#else
esp_thread_info_t fft_cfg_000[] = {
	{
		.run = true,
		.devname = epochs_fft_dev_name,
		.ioctl_req = EPOCHS_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(epochs_fft_cfg_000[0].esp),
	}
};

esp_thread_info_t ifft_cfg_000[] = {
	{
		.run = true,
		.devname = epochs_ifft_dev_name,
		.ioctl_req = EPOCHS_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(epochs_ifft_cfg_000[0].esp),
	}
};
#endif

esp_thread_info_t fir_cfg_000[] = {
	{
		.run = true,
		.devname = fir_dev_name,
		.ioctl_req = AUDIO_FIR_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_fir_cfg_000[0].esp),
	}
};

esp_thread_info_t dma_cfg_000[] = {
	{
		.run = true,
		.devname = dma_dev_name,
		.ioctl_req = AUDIO_DMA_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_dma_cfg_000[0].esp),
	}
};

esp_thread_info_t ffi_cfg_000[] = {
	{
		.run = true,
		.devname = ffi_dev_name,
		.ioctl_req = AUDIO_FFI_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_ffi_cfg_000[0].esp),
	}
};

esp_thread_info_t rotate_cfg_000[] = {
	{
		.run = true,
		.devname = rotate_dev_name,
		.ioctl_req = HU_AUDIODEC_RTL_IOC_ACCESS,
		.esp_desc = &(hu_audiodec_cfg_000[0].esp),
	}
};

#ifdef __cplusplus
}
#endif

#endif // FFI_THREAD_INFO_H
