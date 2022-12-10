#include <stdio.h>
#include <math.h>
#include <BFormat.hpp>
#include <AmbisonicZoomer.hpp>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

void AmbisonicZoomer::Configure(unsigned nChannels) {
    m_nChannelCount = nChannels;

    m_fZoomRed = 0.f;

    m_AmbEncoderFront = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));
    m_AmbEncoderFront_weighted = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));

    /// TODO: Update m_AmbEncoderFront, m_AmbEncoderFront_weighted and m_AmbFrontMic with appropriate values
}

void AmbisonicZoomer::updateZoom() {
    // Limit the zoom value to always preserve the spacial effect.
    /// TODO: Update m_fZoom, m_fZoomRed and m_fZoomBlend with appropriate values
}

void AmbisonicZoomer::Process(CBFormat *pBFSrcDst, unsigned nSamples)
{
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        audio_t fMic = 0.f;

        for(unsigned iChannel=0; iChannel<m_nChannelCount; iChannel++)
        {
            // virtual microphone with polar pattern narrowing as Ambisonic order increases
            fMic += m_AmbEncoderFront_weighted[iChannel] * pBFSrcDst->m_ppfChannels[iChannel][niSample];
        }
        for(unsigned iChannel=0; iChannel<m_nChannelCount; iChannel++)
        {
            if(std::abs(m_AmbEncoderFront[iChannel])>1e-6)
            {
                // Blend original channel with the virtual microphone pointed directly to the front
                // Only do this for Ambisonics components that aren't zero for an encoded frontal source
                pBFSrcDst->m_ppfChannels[iChannel][niSample] = (m_fZoomBlend * pBFSrcDst->m_ppfChannels[iChannel][niSample]
                    + m_AmbEncoderFront[iChannel]*m_fZoom*fMic) / (m_fZoomBlend + std::fabs(m_fZoom)*m_AmbFrontMic);
            }
            else{
                // reduce the level of the Ambisonic components that are zero for a frontal source
                pBFSrcDst->m_ppfChannels[iChannel][niSample] = pBFSrcDst->m_ppfChannels[iChannel][niSample] * m_fZoomRed;
            }
        }
    }
}