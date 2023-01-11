extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <cstring>

#include <BFormat.hpp>

void CBFormat::Configure(unsigned nSampleCount, unsigned nChannels) {
    m_nSamples = nSampleCount;
    m_nChannelCount = nChannels;

    m_nDataLength = nSampleCount * m_nChannelCount;

    // This buffers holds the data for all channels active in the pipeline
    // As described in audio.cpp, all tasks read and write from this.
    m_ppfChannels = (audio_t **) aligned_malloc(m_nChannelCount * sizeof(audio_t *));
    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
        m_ppfChannels[niChannel] = (audio_t *) aligned_malloc(m_nSamples * sizeof(audio_t));
        MyMemset(m_ppfChannels[niChannel], 0, m_nSamples * sizeof(audio_t));
    }
}