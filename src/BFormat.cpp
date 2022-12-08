extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <BFormat.hpp>

void CBFormat::Configure(unsigned nSampleCount, unsigned nChannels) {
    m_nSamples = nSampleCount;
    m_nChannelCount = nChannels;

    m_nDataLength = nSampleCount * m_nChannelCount;

    m_pfData = (float *) aligned_malloc(m_nSamples * m_nChannelCount * sizeof(float));
   
    m_ppfChannels = (float **) aligned_malloc(m_nChannelCount * m_nSamples * sizeof(float));
}