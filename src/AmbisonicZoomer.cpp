#include <stdio.h>
#include <math.h>
#include <AmbisonicZoomer.hpp>

#if (USE_REAL_DATA == 1)
#include <m_AmbEncoderFront.hpp>
#include <m_AmbEncoderFront_weighted.hpp>
#include <ZoomerParams.hpp>
#endif // USE_REAL_DATA

#include <CohDefines.hpp>
#include <ZoomerOptimized.hpp>

void AmbisonicZoomer::Configure(unsigned nChannels) {
    Name = (char *) "ZOOMER";

    m_nChannelCount = nChannels;

    m_fZoomRed = 0.f;

    m_AmbEncoderFront = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));
    m_AmbEncoderFront_weighted = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));

    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
    #if (USE_REAL_DATA == 1)
        m_AmbEncoderFront[niChannel] = Orig_m_AmbEncoderFront[niChannel];
        m_AmbEncoderFront_weighted[niChannel] = Orig_m_AmbEncoderFront[niChannel];
    #else
        m_AmbEncoderFront[niChannel] = myRand();
        m_AmbEncoderFront_weighted[niChannel] = myRand();
    #endif
    }
}

void AmbisonicZoomer::updateZoom() {
    // Limit the zoom value to always preserve the spacial effect.
#if (USE_REAL_DATA == 1)
    m_fZoom = Orig_m_fZoom; 
    m_fZoomRed = Orig_m_fZoomRed;
    m_fZoomBlend = Orig_m_fZoomBlend;
    m_AmbFrontMic = Orig_m_AmbFrontMic;
#else
    m_fZoom = myRand(); 
    m_fZoomRed = myRand();
    m_fZoomBlend = myRand();
    m_AmbFrontMic = myRand();
#endif
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
