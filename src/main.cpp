#include <audio.h>
#include <iostream>
#include <realtime.h>
#include <pthread.h>

#ifndef NATIVE_COMPILE
#define FFT2
#include "hu_audiodec_cfg.h"
#include "fft2_cfg.h"
#include "fft_cfg.h"
#endif

double t_rotatezoom;
double t_decode;
double t_rotate;
double t_zoom;

double t_rotate_acc_mgmt;
double t_rotate_acc;
double t_fft2_acc_mgmt;
double t_fft2_acc;

double t_rotate1;
double t_rotate2;
double t_rotate3;
double t_psycho;
double t_psycho_fft;
double t_psycho_filter;
double t_psycho_ifft;
double t_decode_fft;
double t_decode_filter;
double t_decode_ifft;

double t_total_time;

extern unsigned m_nChannelCount_copy;

unsigned do_fft2_acc_offload;
bool do_rotate_acc_offload;

struct rotate_params {
    float m_fCosAlpha;
    float m_fSinAlpha;
    float m_fCosBeta;
    float m_fSinBeta;
    float m_fCosGamma;
    float m_fSinGamma;
    float m_fCos2Alpha;
    float m_fSin2Alpha;
    float m_fCos2Beta;
    float m_fSin2Beta;
    float m_fCos2Gamma;
    float m_fSin2Gamma;
    float m_fCos3Alpha;
    float m_fSin3Alpha;
    float m_fCos3Beta;
    float m_fSin3Beta;
    float m_fCos3Gamma;
    float m_fSin3Gamma;
} rotate_params_inst;

void rotate_order_acc_offload(CBFormat* pBFSrcDst, unsigned nSamples)
{
#ifndef NATIVE_COMPILE
    clock_t t_start;
    clock_t t_end;
    double t_diff;
    
    t_start = clock();
    size_t size = sizeof(rotate_token_t) * m_nChannelCount_copy * nSamples * 2;
    rotate_token_t *buf = (rotate_token_t *) esp_alloc(size);
	hu_audiodec_cfg_000[0].esp.coherence = ACC_COH_RECALL;
    hu_audiodec_thread_000[0].hw_buf = buf;

	hu_audiodec_cfg_000[0].cfg_regs_8   = float_to_fixed32(rotate_params_inst.m_fCosAlpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_9   = float_to_fixed32(rotate_params_inst.m_fSinAlpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_10  = float_to_fixed32(rotate_params_inst.m_fCosBeta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_11  = float_to_fixed32(rotate_params_inst.m_fSinBeta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_12  = float_to_fixed32(rotate_params_inst.m_fCosGamma, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_13  = float_to_fixed32(rotate_params_inst.m_fSinGamma, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_14  = float_to_fixed32(rotate_params_inst.m_fCos2Alpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_15  = float_to_fixed32(rotate_params_inst.m_fSin2Alpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_16  = float_to_fixed32(rotate_params_inst.m_fCos2Beta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_17  = float_to_fixed32(rotate_params_inst.m_fSin2Beta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_18  = float_to_fixed32(rotate_params_inst.m_fCos2Gamma, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_19  = float_to_fixed32(rotate_params_inst.m_fSin2Gamma, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_20  = float_to_fixed32(rotate_params_inst.m_fCos3Alpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_21  = float_to_fixed32(rotate_params_inst.m_fSin3Alpha, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_22  = float_to_fixed32(rotate_params_inst.m_fCos3Beta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_23  = float_to_fixed32(rotate_params_inst.m_fSin3Beta, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_24  = float_to_fixed32(rotate_params_inst.m_fCos3Gamma, ROTATE_FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_25  = float_to_fixed32(rotate_params_inst.m_fSin3Gamma, ROTATE_FX_IL);

    unsigned out_offset = m_nChannelCount_copy * nSamples;

    // Copying buffer from pBFSrcDst to buf
    for(unsigned niChannel = 1; niChannel < m_nChannelCount_copy; niChannel++)
    {
        for(unsigned niSample = 0; niSample < nSamples; niSample++)
        {
            buf[niChannel*nSamples + niSample] = float_to_fixed32(pBFSrcDst->m_ppfChannels[niChannel][niSample], ROTATE_FX_IL);
        }
    }

    t_end = clock();
    t_diff = double(t_end - t_start);
    t_rotate_acc_mgmt += t_diff;

    // Running the accelerator
    t_start = clock();
    esp_run(hu_audiodec_thread_000, 1);
    t_end = clock();
    t_diff = double(t_end - t_start);
    t_rotate_acc += t_diff;

    t_start = clock();
    // Copying buffer from buf to pBFSrcDst
    for(unsigned niChannel = 1; niChannel < m_nChannelCount_copy; niChannel++)
    {
        for(unsigned niSample = 0; niSample < nSamples; niSample++)
        {
            pBFSrcDst->m_ppfChannels[niChannel][niSample] = (float) fixed32_to_float(buf[out_offset + niChannel*nSamples + niSample], ROTATE_FX_IL);
        }
    }

    esp_free(buf);

    t_end = clock();
    t_diff = double(t_end - t_start);
    t_rotate_acc_mgmt += t_diff;
#endif
}

void fft2_acc_offload(kiss_fft_cfg cfg, const kiss_fft_cpx *fin, kiss_fft_cpx *fout)
{
#ifndef NATIVE_COMPILE
    clock_t t_start;
    clock_t t_end;
    double t_diff;

	const unsigned num_samples = cfg->nfft;
	const unsigned inverse = cfg->inverse;

#ifdef FFT2
	fft2_cfg_000[0].logn_samples = (unsigned) log2(num_samples);
	fft2_cfg_000[0].do_inverse = inverse;
#else
	fft_cfg_000[0].log_len = (unsigned) log2(num_samples);
	fft_cfg_000[0].do_bitrev = DO_BITREV;
#endif

    t_start = clock();
    size_t size = sizeof(fft2_token_t) * 2 * num_ffts * num_samples;
    fft2_token_t *buf = (fft2_token_t *) esp_alloc(size);
#ifdef FFT2
	fft2_cfg_000[0].esp.coherence = ACC_COH_RECALL;
    fft2_thread_000[0].hw_buf = buf;
#else
	fft_cfg_000[0].esp.coherence = ACC_COH_RECALL;
    fft_thread_000[0].hw_buf = buf;
#endif

    // std::cout << "logn_samples " << fft2_cfg_000[0].logn_samples << std::endl;
    // std::cout << "do_inverse " << fft2_cfg_000[0].do_inverse << std::endl;
    // std::cout << "do_shift " << fft2_cfg_000[0].do_shift << std::endl;
    // std::cout << "size " << size << std::endl;

    // Copying buffer from fin to buf
    for(unsigned niSample = 0; niSample < 2 * num_ffts * num_samples; niSample+=2)
    {
        buf[niSample] = float_to_fixed32((fft2_native_t) fin[niSample/2].r, FFT2_FX_IL);
        buf[niSample+1] = float_to_fixed32((fft2_native_t) fin[niSample/2].i, FFT2_FX_IL);
    }

    t_end = clock();
    t_diff = double(t_end - t_start);
    t_fft2_acc_mgmt += t_diff;

    // Running the accelerator
    t_start = clock();
#ifdef FFT2
    esp_run(fft2_thread_000, 1);
#else
    esp_run(fft_thread_000, 1);
#endif
    t_end = clock();
    t_diff = double(t_end - t_start);
    t_fft2_acc += t_diff;

    t_start = clock();
    // Copying buffer from buf to 
    for(unsigned niSample = 0; niSample < 2 * num_ffts * num_samples; niSample+=2)
    {
        fout[niSample/2].r = (float) fixed32_to_float(buf[niSample], FFT2_FX_IL);
        fout[niSample/2].i = (float) fixed32_to_float(buf[niSample+1], FFT2_FX_IL);
    }

    esp_free(buf);

    t_end = clock();
    t_diff = double(t_end - t_start);
    t_fft2_acc_mgmt += t_diff;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

void fft2_acc_offload_wrap(kiss_fft_cfg cfg, const kiss_fft_cpx *fin, kiss_fft_cpx *fout)
{
	fft2_acc_offload(cfg, fin, fout);
}

#ifdef __cplusplus
}
#endif

int main(int argc, char const *argv[])
{
    using namespace ILLIXR_AUDIO;

    t_rotatezoom = 0;
    t_decode = 0;
    t_rotate = 0;
    t_zoom = 0;
    t_rotate_acc_mgmt = 0;
    t_rotate_acc = 0;
    t_rotate1 = 0;
    t_rotate2 = 0;
    t_rotate3 = 0;
    t_psycho = 0;
    t_psycho_fft = 0;
    t_psycho_filter = 0;
    t_psycho_ifft = 0;
    t_decode_fft = 0;
    t_decode_filter = 0;
    t_decode_ifft = 0;
    t_total_time = 0;

    do_fft2_acc_offload = 0;
    do_rotate_acc_offload = false;

    if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <number of size 1024 blocks to process> ";
		std::cout << "<optional: encode/decode>\n";
		std::cout << "Note: If you want to hear the output sound, limit the process sample blocks so that the output is not longer than input!\n";
        return 1;
    }

    const int numBlocks = atoi(argv[1]);
    ABAudio::ProcessType procType(ABAudio::ProcessType::FULL);
    if (argc > 2){
        if (!strcmp(argv[2], "encode"))
            procType = ABAudio::ProcessType::ENCODE;
        else
            procType = ABAudio::ProcessType::DECODE;
    }
    
    ABAudio audio("output.wav", procType);
    audio.loadSource();
    audio.num_blocks_left = numBlocks;

    // Launch realtime audio thread for audio processing
    for (int i = 0; i < numBlocks; ++i) {
        clock_t t_start;
        clock_t t_end;
        double t_diff;

        t_start = clock();
        audio.processBlock();
        t_end = clock();
        t_diff = double(t_end - t_start);
        t_total_time += t_diff;
    }

    std::cout << "High-level:" << std::endl;
    std::cout << "Rotate zoom time " << t_rotatezoom/numBlocks << std::endl;
    std::cout << "Decode time " << t_decode/numBlocks << std::endl;
    std::cout << std::endl;

    std::cout << "Mid-level:" << std::endl;
    std::cout << "Rotate time " << t_rotate/numBlocks << std::endl;
    std::cout << "Zoom time " << t_zoom/numBlocks << std::endl;
    std::cout << "Decode time " << t_decode/numBlocks << std::endl;
    std::cout << std::endl;

    std::cout << "Low-level:" << std::endl;
    std::cout << "Rotate1 time " << t_rotate1/numBlocks << std::endl;
    std::cout << "Rotate2 time " << t_rotate2/numBlocks << std::endl;
    std::cout << "Rotate3 time " << t_rotate3/numBlocks << std::endl;
    std::cout << "psycho time " << t_psycho/numBlocks << std::endl;
    std::cout << "psycho fft time " << t_psycho_fft/numBlocks << std::endl;
    std::cout << "psycho filter time " << t_psycho_filter/numBlocks << std::endl;
    std::cout << "psycho ifft time " << t_psycho_ifft/numBlocks << std::endl;
    std::cout << "decode fft time " << t_decode_fft/numBlocks << std::endl;
    std::cout << "decode filter time " << t_decode_filter/numBlocks << std::endl;
    std::cout << "decode ifft time " << t_decode_ifft/numBlocks << std::endl;
    std::cout << std::endl;

    std::cout << "Acc time:" << std::endl;
    std::cout << "Rotate acc mgmt time " << t_rotate_acc_mgmt/numBlocks << std::endl;
    std::cout << "Rotate acc time " << t_rotate_acc/numBlocks << std::endl;
    std::cout << "fft2 acc mgmt time " << t_fft2_acc_mgmt/numBlocks << std::endl;
    std::cout << "fft2 acc time " << t_fft2_acc/numBlocks << std::endl;

    std::cout << "total time " << t_total_time/numBlocks << std::endl;

    return 0;
}
