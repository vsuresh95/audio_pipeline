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
#define EPOCHS_FFT_STRATUS_IOC_ACCESS  	_IOW ('S', 0, struct epochs_fft_stratus_access)

char fft_dev_name[] = "audio_fft_stratus.0";
char ifft_dev_name[] = "audio_fft_stratus.1";
char fir_dev_name[] = "audio_fir_stratus.0";
char dma_dev_name[] = "audio_dma_stratus.0";
char ffi_dev_name[] = "audio_ffi_stratus.0";
char epochs_fft_dev_name[] = "fft2_stratus.0";
char epochs_ifft_dev_name[] = "fft2_stratus.1";

/* <<--params-def-->> */
#define LOGN_SAMPLES 10
#define DO_SHIFT 0
#define NUM_FFTS     1
#define SCALE_FACTOR 1
#define START_OFFSET 1

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

#ifdef __cplusplus
}
#endif

#endif // FFI_THREAD_INFO_H
