#include "doctest.h"
#include <Gist.h>

//=============================================================
//=================== SPECTRAL DIFFERENCE =====================
//=============================================================
TEST_SUITE ("SpectralDifference")
{
    // ------------------------------------------------------------
    // 1. Check that a buffer of zeros returns zero on two occasions
    TEST_CASE ("Zero_Test")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf(frameSize);
        
        std::vector<float> testSpectrum (512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = 0;
        
        // first time
        float r = odf.spectralDifference (testSpectrum);

        CHECK_EQ (r, 0);
        
        // second time
        r = odf.spectralDifference (testSpectrum);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 2. Check that a buffer of ones returns the frame size the first time
    // and zero the second time
    TEST_CASE ("Ones_Test")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf (frameSize);
        
        std::vector<float> testSpectrum(512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = 1;
        
        // first time
        float r = odf.spectralDifference (testSpectrum);
        
        CHECK_EQ (r, frameSize);
        
        // second time
        r = odf.spectralDifference (testSpectrum);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 3. Numerical Test
    TEST_CASE ("NumericalTest")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf(frameSize);
        
        std::vector<float> testSpectrum(512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = i;
        
        // first time
        float r = odf.spectralDifference (testSpectrum);
        
        CHECK_EQ (r, 130816);
        
        for (int i = 0; i < 512; i++)
        {
            testSpectrum[i] = 1;
        }
        
        // second time
        r = odf.spectralDifference (testSpectrum);
        
        CHECK_EQ (r, 130306);
        
    }
}

//=============================================================
//========= SPECTRAL DIFFERENCE (HALF WAVE RECTIFIED) =========
//=============================================================
TEST_SUITE ("SpectralDifferenceHWR")
{
    // ------------------------------------------------------------
    // 1. Check that a buffer of zeros returns zero on two occasions
    TEST_CASE ("Zero_Test")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf(frameSize);
        
        std::vector<float> testSpectrum (512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = 0;
        
        // first time
        float r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, 0);
        
        // second time
        r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 2. Check that a buffer of ones returns the frame size the first time
    // and zero the second time
    TEST_CASE ("Ones_Test")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf(frameSize);
        
        std::vector<float> testSpectrum (512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = 1;
        
        // first time
        float r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, frameSize);
        
        // second time
        r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 3. Numerical Test
    TEST_CASE ("NumericalTest")
    {
        int frameSize = 512;
        
        OnsetDetectionFunction<float> odf(frameSize);
        
        std::vector<float> testSpectrum (512);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = i;
        
        // first time
        float r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, 130816);
        
        for (int i = 0; i < 512; i++)
            testSpectrum[i] = 0;
        
        // second time
        r = odf.spectralDifferenceHWR (testSpectrum);
        
        CHECK_EQ (r, 0);
    }
}
