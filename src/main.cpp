#include <audio.h>
#include <iostream>
#include <realtime.h>
#include <pthread.h>

#include "hu_audiodec_cfg.h"

double t_rotatezoom;
double t_decode;
double t_rotate;
double t_zoom;

double t_rotate_acc_mgmt;
double t_rotate_acc;
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

#define FX_IL 16

void rotate_order_acc_offload(CBFormat* pBFSrcDst, unsigned nSamples)
{
    clock_t t_start;
    clock_t t_end;
    double t_diff;
    
    t_start = clock();
    size_t size = sizeof(token_t) * m_nChannelCount_copy * nSamples * 2;
    token_t *buf = (token_t *) esp_alloc(size);
	hu_audiodec_cfg_000[0].esp.coherence = ACC_COH_RECALL;
    cfg_000[0].hw_buf = buf;

    unsigned out_offset = m_nChannelCount_copy * nSamples;

    // Copying buffer from pBFSrcDst to buf
    for(unsigned niChannel = 0; niChannel < m_nChannelCount_copy; niChannel++)
    {
        for(unsigned niSample = 0; niSample < nSamples; niSample++)
        {
            buf[niChannel*nSamples + niSample] = float_to_fixed32(pBFSrcDst->m_ppfChannels[niChannel][niSample], FX_IL);

            // std::cout << "Channel " << niChannel;
            // std::cout << " Sample " << niSample;
            // std::cout << " Input " << pBFSrcDst->m_ppfChannels[niChannel][niSample];
            // std::cout << " Output " << buf[niChannel*nSamples + niSample];
            // std::cout << std::endl;
        }
    }

    t_end = clock();
    t_diff = double(t_end - t_start);
    t_rotate_acc_mgmt += t_diff;

    // Running the accelerator
    t_start = clock();
    esp_run(cfg_000, 1);
    t_end = clock();
    t_diff = double(t_end - t_start);
    t_rotate_acc += t_diff;

    // Copying buffer from pBFSrcDst to buf
    for(unsigned niChannel = 0; niChannel < m_nChannelCount_copy; niChannel++)
    {
        for(unsigned niSample = 0; niSample < nSamples; niSample++)
        {
            pBFSrcDst->m_ppfChannels[niChannel][niSample] = (float) fixed32_to_float(buf[out_offset + niChannel*nSamples + niSample], FX_IL);

            // std::cout << "Channel " << niChannel;
            // std::cout << " Sample " << niChannel;
            // std::cout << " Input " << buf[out_offset + niChannel*nSamples + niSample];
            // std::cout << " Output " << pBFSrcDst->m_ppfChannels[niChannel][niSample];
            // std::cout << std::endl;
        }
    }

    esp_free(buf);
}

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
    std::cout << "Rotate acc mgmt time " << t_rotate_acc_mgmt/numBlocks << std::endl;
    std::cout << "Rotate acc time " << t_rotate_acc/numBlocks << std::endl;
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

    std::cout << "total time " << t_total_time/numBlocks << std::endl;

    return 0;
}
