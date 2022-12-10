extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <BFormat.hpp>

void CBFormat::Configure(unsigned nSampleCount, unsigned nChannels) {
    m_nSamples = nSampleCount;
    m_nChannelCount = nChannels;

    m_nDataLength = nSampleCount * m_nChannelCount;

    m_pfData = (audio_t *) aligned_malloc(m_nSamples * m_nChannelCount * sizeof(audio_t));
   
    m_ppfChannels = (audio_t **) aligned_malloc(m_nChannelCount * sizeof(audio_t *));
    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
        m_ppfChannels[niChannel] = (audio_t *) aligned_malloc(m_nSamples * sizeof(audio_t));
    }
}