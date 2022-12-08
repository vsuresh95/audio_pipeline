#ifndef ZOOMER_H
#define ZOOMER_H

class AmbisonicZoomer {
public:
    unsigned m_nChannelCount;

    float *m_AmbEncoderFront;
    float *m_AmbEncoderFront_weighted;

    float m_fZoom;
    float m_fZoomRed;
    float m_AmbFrontMic;
    float m_fZoomBlend;

    void Configure(unsigned nChannels);

    void updateZoom();

    void Process(CBFormat *pBFSrcDst, unsigned nSamples);
};

#endif // ZOOMER_H