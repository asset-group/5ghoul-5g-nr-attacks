#include "doctest.h"
#include <Gist.h>
#include "Test_Signals.h"

//=============================================================
//========================= GIST ==============================
//=============================================================
TEST_SUITE ("GistTest")
{
    //=============================================================
    TEST_CASE ("TestFFT1")
    {
        Gist<float> g (512, 44100, WindowType::RectangularWindow);
        
        std::vector<float> testFrame(512);
        
        for (int i = 0;i < 512;i++)
        {
            testFrame[i] = 0;
        }
        
        testFrame[0] = 1.0;
        
        g.processAudioFrame(testFrame);
        
        std::vector<float> mag;
        
        mag = g.getMagnitudeSpectrum();
        
        for (size_t i = 0; i < mag.size(); i++)
        {
            CHECK_EQ(mag[i], 1.0);
        }
    }

    //=============================================================
    TEST_CASE ("TestFFT2")
    {
        Gist<float> g(512, 44100, WindowType::RectangularWindow);
            
        std::vector<float> testFrame(512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = 1.0;
        
        g.processAudioFrame (testFrame);
        
        std::vector<float> mag;
        
        mag = g.getMagnitudeSpectrum();
        
        CHECK_EQ (mag[0], testFrame.size());
        
        for (size_t i = 1; i < mag.size(); i++)
            CHECK_EQ (mag[i], 0.0);
    }

    //=============================================================
    TEST_CASE ("TestFFT3")
    {
        Gist<float> g (256, 44100, WindowType::RectangularWindow);
        
        std::vector<float> testFrame(256);
        
        for (int i = 0; i < 256; i++)
            testFrame[i] = fftTestIn[i];
        
        g.processAudioFrame(testFrame);
        
        std::vector<float> mag;
        
        mag = g.getMagnitudeSpectrum();
        
        for (size_t i = 1; i < mag.size(); i++)
        {
            CHECK (mag[i] == doctest::Approx (fftTestMag[i]).epsilon (0.001));
        }
    }


    //=============================================================
    TEST_CASE ("TestFFT4")
    {
        Gist<double> g (256, 44100, WindowType::RectangularWindow);
        
        std::vector<double> testFrame(256);
        
        for (int i = 0; i < 256; i++)
            testFrame[i] = fftTestIn[i];
        
        g.processAudioFrame (testFrame);
        
        std::vector<double> mag;
        
        mag = g.getMagnitudeSpectrum();
        
        for (size_t i = 1; i < mag.size(); i++)
        {
            CHECK (mag[i] == doctest::Approx (fftTestMag[i]).epsilon (0.001));
        }
    }


    //=============================================================
    TEST_CASE ("RMS_Test")
    {
        Gist<float> g(512,44100);
        
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame (512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = ((float)((rand() % 1000) - 500)) / 1000.;
        
        g.processAudioFrame (testFrame);
        
        float r1 = g.rootMeanSquare();
        float r2 = tdf.rootMeanSquare(testFrame);
        CHECK_EQ (r1, r2);
    }

    //=============================================================
    TEST_CASE ("PeakEnergy_Test")
    {
        Gist<float> g(512,44100);
        
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(512);
        
        for (int i = 0;i < 512;i++)
        {
            testFrame[i] = ((float)((rand() % 1000) - 500)) / 1000.;
        }
        
        g.processAudioFrame (testFrame);
        
        float r1 = g.peakEnergy();
        
        float r2 = tdf.peakEnergy (testFrame);
        
        CHECK_EQ (r1, r2);
        
    }

    //=============================================================
    TEST_CASE ("ZeroCrossingRate_Test")
    {
        Gist<float> g (512, 44100);
        
        CoreTimeDomainFeatures<float> tdf;
        
        float testFrame[512];
        
        for (int i = 0;i < 512;i++)
        {
            testFrame[i] = ((float)((rand() % 1000) - 500)) / 1000.;
        }
        
        g.processAudioFrame (testFrame, 512);
        
        float r1 = g.zeroCrossingRate();
        
        std::vector<float> v;
        v.assign (testFrame, testFrame + 512);
        float r2 = tdf.zeroCrossingRate (v);
        
        CHECK_EQ (r1, r2);
        
    }

    //=============================================================
    TEST_CASE ("SpectralCentroid_Test")
    {
        Gist<float> g (512,44100);
        
        CoreFrequencyDomainFeatures<float> fdf;
        
        float testFrame [512];
        
        for (int i = 0;i < 512;i++)
        {
            testFrame[i] = ((float)((rand() % 1000) - 500)) / 1000.;
        }
        
        g.processAudioFrame (testFrame, 512);
        
        float r1 = g.spectralCentroid();
        float r2 = fdf.spectralCentroid (g.getMagnitudeSpectrum());
        
        CHECK_EQ (r1, r2);
    }
}
