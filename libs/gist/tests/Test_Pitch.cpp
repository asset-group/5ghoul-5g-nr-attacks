#include "doctest.h"
#include <Gist.h>
#include "Test_Signals.h"

//=============================================================
//========================= YIN ===============================
//=============================================================
TEST_SUITE ("PitchYin")
{
    // ------------------------------------------------------------
    // 1. CHECK KNOWN BUFFER WITH KNOWN PITCH 1
    TEST_CASE ("Buffer_Test_1")
    {
        Yin<float> y (44100);
        
        std::vector<float> frame;
        
        for (int i = 0; i < 512; i++)
            frame.push_back(pitchTest1[i]);
        
        float r = y.pitchYin (frame);
        
        //BOOST_CHECK_CLOSE(r,pitchTest1_result,0.0001);
        CHECK (r == doctest::Approx (pitchTest1_result).epsilon (0.0001));
    }

    // ------------------------------------------------------------
    // 2. CHECK KNOWN BUFFER WITH KNOWN PITCH 2
    TEST_CASE ("Buffer_Test_2")
    {
        Yin<float> y (44100);
        
        std::vector<float> frame;
        
        for (int i = 0; i < 512; i++)
            frame.push_back (pitchTest2[i]);
        
        float r = y.pitchYin (frame);
        
        //BOOST_CHECK_CLOSE(r,pitchTest2_result,0.0001);
        CHECK (r == doctest::Approx (pitchTest2_result).epsilon (0.0001));
    }

    // ------------------------------------------------------------
    // 3. CHECK BUFFER OF ZEROS
    TEST_CASE ("ZeroTest")
    {
        Yin<float> y (44100);
        
        std::vector<float> frame;
        
        for (int i = 0; i < 512; i++)
            frame.push_back (0.0);
        
        float r = y.pitchYin (frame);
            
        CHECK_EQ (r, y.getMaxFrequency());
    }

    // ------------------------------------------------------------
    // 4. CHECK BUFFER OF ONES (i.e. positive flat signal)
    TEST_CASE ("OnesTest")
    {
        Yin<float> y (44100);
        
        std::vector<float> frame;
        
        for (int i = 0; i < 512; i++)
            frame.push_back (1.0);
        
        float r = y.pitchYin (frame);
        
        CHECK_EQ (r, y.getMaxFrequency());
    }

    // ------------------------------------------------------------
    // 5. CHECK BUFFER OF NEGATIVE ONES (i.e. negative flat signal)
    TEST_CASE ("NegativeOnesTest")
    {
        Yin<float> y (44100);
        
        std::vector<float> frame;
        
        for (int i = 0; i < 512; i++)
            frame.push_back(-1.0);
        
        float r = y.pitchYin (frame);
        
        CHECK_EQ (r, y.getMaxFrequency());
    }
}
