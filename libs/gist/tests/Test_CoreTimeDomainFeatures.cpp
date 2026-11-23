#include "doctest.h"
#include <Gist.h>

//=============================================================
//========================= RMS ===============================
//=============================================================
TEST_SUITE ("RMS")
{
    // ------------------------------------------------------------
    // 1. Check that a buffer of zeros returns zero
    TEST_CASE ("Zero_Test")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame (512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = 0;
        
        float r = tdf.rootMeanSquare(testFrame);
        
        CHECK_EQ (r,0);
        
    }

    // ------------------------------------------------------------
    // 2. Check that a buffer of all ones returns 1
    TEST_CASE ("Ones_Test")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = 1;
        
        float r = tdf.rootMeanSquare(testFrame);
        
        CHECK_EQ (r, 1);
        
    }

    // ------------------------------------------------------------
    // 3. Numeric Example
    TEST_CASE ("Numeric_Example")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(6);
        
        testFrame[0] = 0;
        testFrame[1] = 1;
        testFrame[2] = 2;
        testFrame[3] = 3;
        testFrame[4] = 4;
        testFrame[5] = 5;
        
        float r = tdf.rootMeanSquare(testFrame);
        
        CHECK (r == doctest::Approx (3.0276503540974917).epsilon (0.01));
    }
}


//=============================================================
//===================== PEAK ENERGY ===========================
//=============================================================
TEST_SUITE ("PeakEnergy")
{
    // ------------------------------------------------------------
    // 1. Check that a buffer of zeros returns zero
    TEST_CASE ("Zero_Test")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame (512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = 0;
        
        float r = tdf.peakEnergy (testFrame);
        
        CHECK_EQ (r,0);
        
    }

    // ------------------------------------------------------------
    // 2. Check that passing the index returns the highest index
    TEST_CASE ("Max_Val_Test_1")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        int frameSize = 512;
        
        std::vector<float> testFrame (frameSize);
        
        for (int i = 0; i < frameSize; i++)
            testFrame[i] = (float) i;
        
        float r = tdf.peakEnergy (testFrame);
        
        CHECK_EQ (r, frameSize - 1);
        
    }

    // ------------------------------------------------------------
    // 3. Numeric Example 1
    TEST_CASE ("Numeric_1")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        int frameSize = 6;
        
        std::vector<float> testFrame(frameSize);
        
        testFrame[0] = 0;
        testFrame[1] = 10;
        testFrame[2] = 2;
        testFrame[3] = 37;
        testFrame[4] = 17;
        testFrame[5] = 19;
        
        float r = tdf.peakEnergy (testFrame);
        
        CHECK_EQ (r, 37);
    }
}

//=============================================================
//===================== ZERO CROSSING RATE ====================
//=============================================================
TEST_SUITE ("ZeroCrossingRate")
{
    // ------------------------------------------------------------
    // 1. Check that a buffer of zeros returns zero
    TEST_CASE ("Zero_Test")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame (512);
        
        for (int i = 0; i < 512; i++)
            testFrame[i] = 0;
        
        float r = tdf.zeroCrossingRate (testFrame);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 2. Check that a buffer that is all larger than 0 returns 0
    TEST_CASE ("LargerThanZero")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(512);
        
        for (int i = 0; i < 512; i++)
        {
            testFrame[i] = ((float) i+1) / 512;
        }
        
        float r = tdf.zeroCrossingRate(testFrame);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 3. Check that a buffer that is all 0 or less returns 0
    TEST_CASE ("ZeroOrLess")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(512);
        
        for (int i = 0; i < 512; i++)
        {
            testFrame[i] = ((float) -i) / 512;
        }
        
        float r = tdf.zeroCrossingRate(testFrame);
        
        CHECK_EQ (r, 0);
        
    }

    // ------------------------------------------------------------
    // 4. NumericExample - check that three zero crossings produces 3 as a return value
    TEST_CASE ("NumericExample")
    {
        CoreTimeDomainFeatures<float> tdf;
        
        std::vector<float> testFrame(6);
        
        testFrame[0] = 0;
        testFrame[1] = 0.1;
        testFrame[2] = 0.4;
        testFrame[3] = -0.3;
        testFrame[4] = -0.1;
        testFrame[5] = 0.3;

        float r = tdf.zeroCrossingRate(testFrame);
        
        CHECK_EQ (r, 3);
    }
}
