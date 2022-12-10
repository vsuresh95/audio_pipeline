#ifndef ZOOMER_H
#define ZOOMER_H

class AmbisonicZoomer {
public:
    unsigned m_nChannelCount;

    audio_t *m_AmbEncoderFront;
    audio_t *m_AmbEncoderFront_weighted;

    audio_t m_fZoom;
    audio_t m_fZoomRed;
    audio_t m_AmbFrontMic;
    audio_t m_fZoomBlend;

    void Configure(unsigned nChannels);

    void updateZoom();

    void Process(CBFormat *pBFSrcDst, unsigned nSamples);
};

#endif // ZOOMER_H