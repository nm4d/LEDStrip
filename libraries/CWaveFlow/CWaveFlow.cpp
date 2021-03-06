// includes
#include "LPD8806.h"
#include "CWaveFlow.h"

// function implementations
//CWaveFlow::CWaveFlow(unsigned int auShift[3], LPD8806* alLedStrip, unsigned int auNumLeds,
//                     unsigned int auWindowSize, unsigned int auSigmaGauss, unsigned int auAmplGauss):
//    m_uPosition(auShift), m_lLedStrip(alLedStrip), m_uNumLeds(auNumLeds), m_uWindowSize(auWindowSize),
//    m_uSigmaGauss(auSigmaGauss), m_uAmplGauss(auAmplGauss)
//{
//    for(unsigned int i = 0; i < 3; i++)
//    {
//        m_eWFDir[i] = eWaveFlowDirFwd;
//        m_uIntensity[i] = new(unsigned int [2 * auWindowSize + m_uNumLeds]);
//        m_uLedStripIntensity[i] = new(unsigned int [m_uNumLeds]);
//        for(unsigned int j = 0; j < m_uNumLeds + 2 * auWindowSize; j++)
//        {
//            m_uIntensity[i][j] = 0;
//        }
//    }
//}

CWaveFlow::CWaveFlow()
{
    m_uWindowSize = 1;
    m_uAmplitude = 0xEF;
    m_lLedStrip = 0;
    
    for(unsigned int i = 0; i < 3; i++)
    {
        m_uPosition[i] = 0;
        m_uIntensity[i] = 0;
        m_uLedStripIntensity[i] = 0;;
    }
    m_eWFDir[0] = eWaveFlowDirBwd;
    m_eWFDir[1] = eWaveFlowDirFwd;
    m_eWFDir[2] = eWaveFlowDirFwd;
    m_uAmbientRunsReset = 0;
    m_uAmbientRuns = 0;
    this->constrainLedStrip();
//    this->show();
}

CWaveFlow::~CWaveFlow()
{
    delete[] m_uIntensity;
    delete[] m_uLedStripIntensity;
}

void CWaveFlow::setShift(unsigned int* auShift)
{
    m_uPosition[0] = auShift[0];
    m_uPosition[1] = auShift[1];
    m_uPosition[2] = auShift[2];
}

void CWaveFlow::setSkip(unsigned int* aiSkip)
{
    m_uSkipReset[0] = aiSkip[0];
    m_uSkipReset[1] = aiSkip[1];
    m_uSkipReset[2] = aiSkip[2];
    m_uSkip[0] = aiSkip[0];
    m_uSkip[1] = aiSkip[1];
    m_uSkip[2] = aiSkip[2];
}


void CWaveFlow::setLedStrip(LPD8806* alLedStrip)
{
    m_lLedStrip = alLedStrip;
}

void CWaveFlow::setNumLeds(unsigned int auNumLeds)
{
    m_uNumLeds = auNumLeds;
}

void CWaveFlow::setWindowSize(unsigned int auWindowSize)
{
    m_uWindowSize = auWindowSize;
}

void CWaveFlow::setAmplitude(uint8_t auAmplitude)
{
    m_uAmplitude = auAmplitude;
}

void CWaveFlow::setMode(EMode aeMod)
{
    m_eMod = aeMod;
}

void CWaveFlow::setAmbientRuns(uint8_t auRuns)
{
    m_uAmbientRunsReset = auRuns;
}
//calculate gaussian window
void CWaveFlow::calcIntensity()
{
    if (m_eMod == eModFlow) {
        m_uLengthIntensity = m_uWindowSize + m_uNumLeds;
        m_uOverlap = (m_uLengthIntensity - m_uNumLeds)/2;
        
        for(unsigned int i = 0; i < 3; i++)
        {
            m_uIntensity[i] = new(unsigned int [m_uLengthIntensity]);
            m_uLedStripIntensity[i] = new(unsigned int [m_uNumLeds]);
            for(unsigned int j = 0; j < m_uLengthIntensity - 1; j++)
            {
                m_uIntensity[i][j] = 0U;
            }
        }
        for(unsigned int i = 0; i <= (m_uWindowSize + 1)/2; i++)
        {
            m_uIntensity[0][i] = 2 * i * (m_uAmplitude / (m_uWindowSize + 1));
            m_uIntensity[1][i] = m_uIntensity[0][i];
            m_uIntensity[2][i] = m_uIntensity[0][i];
        }

        for(unsigned int i = (m_uWindowSize + 1)/2 + 1; i < m_uWindowSize; i++)
        {
            
            m_uIntensity[0][i] = m_uAmplitude - (2 * m_uAmplitude / (m_uWindowSize + 1)) * (i - ((m_uWindowSize + 1) / 2));
            m_uIntensity[1][i] = m_uIntensity[0][i];
            m_uIntensity[2][i] = m_uIntensity[0][i];
        }
    }
    else if (m_eMod == eModAmbient)
    {
        if (m_uAmbientRuns == 0)
        {
            m_uAmbientRuns = m_uAmbientRunsReset;
            for(uint8_t i = 0; i < m_uNumLeds; i++)
            {
                m_uLedStripIntensity[0][i] = 0;
                m_uLedStripIntensity[1][i] = 0;
                m_uLedStripIntensity[2][i] = 0;
            }
            randomSeed(analogRead(0));
            m_uAmbientCol = random(1000)%3;
        }
        else
        {
            for (uint8_t i = 0; i < m_uNumLeds; i++) {
                if (m_uAmbientRuns > (m_uAmbientRunsReset / 2))
                {
                    m_uLedStripIntensity[m_uAmbientCol][i] = (0xFF/m_uAmbientRunsReset) * (m_uAmbientRunsReset - m_uAmbientRuns);
                } else {
                    m_uLedStripIntensity[m_uAmbientCol][i] = 0xFF + (0xFF/m_uAmbientRunsReset) * (m_uAmbientRuns - 1 - (m_uAmbientRunsReset / 2));
                }
            }
            m_uAmbientRuns--;
        }
    }
}

//initial shift between r, g, b
void CWaveFlow::applyShift()
{
    unsigned int uShift[3] = {0, 0, 0};
    
    for(unsigned int i = 0; i < 3; i++)
    {
        uShift[i] = m_uPosition[i]+1;
        do
        {
            for (unsigned int j = m_uLengthIntensity - 1; j > 0 ; j--)
            {
                m_uIntensity[i][j] = m_uIntensity[i][j-1];
                if(j == 1)
                {
                    m_uIntensity[i][0] = 0;
                }
            }
            --uShift[i];
        } while (uShift[i] > 0);
    }
    this->constrainLedStrip();
}

void CWaveFlow::constrainLedStrip()
{
    for(unsigned int i = 0; i < 3; i++)
    {
        for(unsigned int j = 0; j < m_uLengthIntensity - 1; j++)
        {
            if((j > m_uOverlap) && (j < (m_uLengthIntensity - m_uOverlap)))
            {
                m_uLedStripIntensity[i][j - m_uOverlap - 1] = m_uIntensity[i][j];
            }
        }

    }

}

void CWaveFlow::moveIntensity()
{
    if (m_eMod == eModFlow) {
        for(unsigned int i = 0; i < 3; i++)
        {
            
            if (m_uSkip[i] > 0)
            {
                m_uSkip[i]--;
            }
            else
            {
                m_uSkip[i] = m_uSkipReset[i];
                
                    if(m_eWFDir[i] == eWaveFlowDirFwd)
                    {
                        for(unsigned int jFwd = m_uLengthIntensity - 1; jFwd > 0; jFwd--)
                        {
                            if(jFwd == 1)
                            {
                                m_uIntensity[i][0] = 0;
                            }
                                m_uIntensity[i][jFwd] = m_uIntensity[i][jFwd - 1];
                        }
                        ++(m_uPosition[i]);
                        if(m_uPosition[i] == m_uLengthIntensity -  m_uOverlap - 1)
                        {
                            m_eWFDir[i] = eWaveFlowDirBwd;
                        }
                    }
                    else
                    {
                        for(unsigned int jBwd = 0; jBwd <  m_uLengthIntensity - 1; jBwd++)
                        {
                            if(jBwd == m_uLengthIntensity - 2)
                            {
                                m_uIntensity[i][m_uLengthIntensity - 1] = 0;
                            }
                            m_uIntensity[i][jBwd] = m_uIntensity[i][jBwd + 1];
                    
                        }
                        --(m_uPosition[i]);
                        if(m_uPosition[i] == 0)
                        {
                            m_eWFDir[i] = eWaveFlowDirFwd;
                        }
                    }
                }
            }
        this->constrainLedStrip();
    }
    else if (m_eMod == eModAmbient)
    {
        this->calcIntensity();
    }
    this->show();
}


void CWaveFlow::show()
{
    for (unsigned int i = 0; i < m_uNumLeds; i++) {
//        m_lLedStrip->setPixelColor(i, m_uLedStripIntensity[0][i], m_uLedStripIntensity[1][i], m_uLedStripIntensity[2][i]);
        m_lLedStrip->setPixelColor(i, 0, m_uLedStripIntensity[1][i], 0);
    }
    m_lLedStrip->show();
}
