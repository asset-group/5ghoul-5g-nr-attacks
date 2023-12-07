#include "doctest.h"
#include <Gist.h>
#include "Test_Signals.h"

//=============================================================
//========================== MFCC =============================
//=============================================================
TEST_SUITE ("MFCC_test")
{
    // ------------------------------------------------------------
    // 1. CHECK EXAMPLE MAGNITUDE SPECTRUM WITH KNOWN RESULT
    TEST_CASE ("ExampleMagnitudeSpectrumTest")
    {
        MFCC<float> mfcc (512, 44100);
        
        mfcc.setNumCoefficients (13);
        
        std::vector<float> magnitudeSpecV (256);
        
        for (int i = 0; i < 256; i++)
            magnitudeSpecV[i] = magnitudeSpectrum[i];
        
        mfcc.calculateMelFrequencyCepstralCoefficients (magnitudeSpecV);
        
        for (size_t i = 0; i < mfcc.MFCCs.size(); i++)
        {
            CHECK (mfcc.MFCCs[i] == doctest::Approx (mfccTest1_result[i]).epsilon (0.01));
        }
    }
}
