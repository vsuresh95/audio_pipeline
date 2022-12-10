#ifndef BFORMAT_H
#define BFORMAT_H

class CBFormat {
public:
    unsigned m_nSamples;
    unsigned m_nDataLength;
    unsigned m_nChannelCount;
    audio_t *m_pfData;
    audio_t **m_ppfChannels;

    void Configure(unsigned nSampleCount, unsigned nChannels);
};

enum BFormatChannels3D
{
    kW,
    kY, kZ, kX,
    kV, kT, kR, kS, kU,
    kQ, kO, kM, kK, kL, kN, kP,
    kNumOfBformatChannels3D
};

#endif // BFORMAT_H