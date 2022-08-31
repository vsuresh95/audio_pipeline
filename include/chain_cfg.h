#ifndef __CHAIN_CFG_H__
#define __CHAIN_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libesp.h>
#include <esp.h>
#include <esp_accelerator.h>

#define FFT2_FX_IL	 4
#define NUM_FIRS     1
#define NUM_FFTS     1
#define LOGN_SAMPLES 10
#define DO_SHIFT     0
#define SCALE_FACTOR 0

#define SYNC_VAR_SIZE 2
#define NUM_DEVICES 3

struct kiss_fftr_state{
    kiss_fft_cfg substate;
    kiss_fft_cpx * tmpbuf;
    kiss_fft_cpx * super_twiddles;
#ifdef USE_SIMD
    void * pad;
#endif
};

struct fft2_stratus_access chain_fft_cfg_000[] = {
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

struct fft2_stratus_access chain_fft_cfg_001[] = {
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

struct fir_stratus_access chain_fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.scale_factor = SCALE_FACTOR,
		.do_inverse = 0,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.num_firs = NUM_FIRS,
		.src_offset = 0,
		.dst_offset = 0,
	}
};

#define CHAIN_FFT_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fft2_stratus_access)
#define CHAIN_FIR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct fir_stratus_access)

char chain_fft2_0_dev_name[] = "fft2_stratus.0";
char chain_fft2_1_dev_name[] = "fft2_stratus.1";
char chain_fir_dev_name[] = "fir_stratus.0";

esp_thread_info_t chain_thread_000[] = {
	{
		.run = true,
		.devname = chain_fft2_0_dev_name,
		.ioctl_req = CHAIN_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(chain_fft_cfg_000[0].esp),
	}
};

esp_thread_info_t chain_thread_001[] = {
	{
		.run = true,
		.devname = chain_fft2_0_dev_name,
		.ioctl_req = CHAIN_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(chain_fft_cfg_001[0].esp),
	}
};

esp_thread_info_t chain_thread_002[] = {
	{
		.run = true,
		.devname = chain_fir_dev_name,
		.ioctl_req = CHAIN_FIR_STRATUS_IOC_ACCESS,
		.esp_desc = &(chain_fir_cfg_000[0].esp),
	}
};

#ifdef __cplusplus
}
#endif

#endif /* __CHAIN_CFG_H__ */