//=======================================================================
/** @file AccelerateFFT.cpp
 *  @brief Performs the FFT using the Apple Accelerate Framework
 *  @author Adam Stark
 *  @copyright Copyright (C) 2013  Adam Stark
 *
 * This file is part of the 'Gist' audio analysis library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//=======================================================================

#include "AccelerateFFT.h"
#include <assert.h>

#ifdef USE_ACCELERATE_FFT

//=======================================================================
template <class T>
AccelerateFFT<T>::AccelerateFFT()
{
    complexSplit.realp = nullptr;
    complexSplit.imagp = nullptr;
    doubleComplexSplit.realp = nullptr;
    doubleComplexSplit.imagp = nullptr;
    
    configured = false;
}


//=======================================================================
template <>
void AccelerateFFT<float>::setAudioFrameSize (int frameSize)
{    
    fftSize = frameSize;
    fftSizeOver2 = fftSize / 2;
    log2n = log2f (fftSize);

    if (configured)
    {
        free (complexSplit.realp);
        free (complexSplit.imagp);
        vDSP_destroy_fftsetup (fftSetupFloat);
    }
    
    complexSplit.realp = (float*)malloc (fftSize * sizeof (float));
    complexSplit.imagp = (float*)malloc (fftSize * sizeof (float));
        
    fftSetupFloat = vDSP_create_fftsetup (log2n, FFT_RADIX2);
    
    if (fftSetupFloat == nullptr)
    {
        // couldn't set up FFT
        assert (false);
    }
    
    configured = true;
}

//=======================================================================
template <>
void AccelerateFFT<double>::setAudioFrameSize (int frameSize)
{
    fftSize = frameSize;
    fftSizeOver2 = fftSize / 2;
    log2n = log2f (fftSize);
    
    if (configured)
    {
        free (doubleComplexSplit.realp);
        free (doubleComplexSplit.imagp);
        vDSP_destroy_fftsetupD (fftSetupDouble);
    }
    
    doubleComplexSplit.realp = (double*)malloc (fftSize * sizeof (double));
    doubleComplexSplit.imagp = (double*)malloc (fftSize * sizeof (double));
    
    fftSetupDouble = vDSP_create_fftsetupD (log2n, FFT_RADIX2);
    
    if (fftSetupDouble == nullptr)
    {
        // couldn't set up FFT
        assert (false);
    }
    
    configured = true;
}

//=======================================================================
template <>
AccelerateFFT<float>::~AccelerateFFT()
{
    free (complexSplit.realp);
    free (complexSplit.imagp);
    vDSP_destroy_fftsetup (fftSetupFloat);
}

//=======================================================================
template <>
AccelerateFFT<double>::~AccelerateFFT()
{
    free (doubleComplexSplit.realp);
    free (doubleComplexSplit.imagp);
    vDSP_destroy_fftsetupD (fftSetupDouble);
}

//=======================================================================
template <>
void AccelerateFFT<float>::performFFT (float* buffer, float* real, float* imag)
{
    vDSP_ctoz ((COMPLEX*)buffer, 2, &complexSplit, 1, fftSizeOver2);
    vDSP_fft_zrip (fftSetupFloat, &complexSplit, 1, log2n, FFT_FORWARD);
    
    complexSplit.realp[fftSizeOver2] = complexSplit.imagp[0];
    complexSplit.imagp[fftSizeOver2] = 0.0;
    complexSplit.imagp[0] = 0.0;
    
    for (size_t i = 0; i <= fftSizeOver2; i++)
    {
        complexSplit.realp[i] *= 0.5;
        complexSplit.imagp[i] *= 0.5;
    }
    
    for (size_t i = fftSizeOver2 - 1; i > 0; --i)
    {
        complexSplit.realp[2 * fftSizeOver2 - i] = complexSplit.realp[i];
        complexSplit.imagp[2 * fftSizeOver2 - i] = -1 * complexSplit.imagp[i];
    }
    
    for (size_t i = 0; i < fftSize; i++)
    {
        real[i] = complexSplit.realp[i];
        imag[i] = complexSplit.imagp[i];
    }
}

//=======================================================================
template <>
void AccelerateFFT<double>::performFFT (double* buffer, double* real, double* imag)
{
    vDSP_ctozD ((DOUBLE_COMPLEX*)buffer, 2, &doubleComplexSplit, 1, fftSizeOver2);
    vDSP_fft_zripD (fftSetupDouble, &doubleComplexSplit, 1, log2n, FFT_FORWARD);
    
    doubleComplexSplit.realp[fftSizeOver2] = doubleComplexSplit.imagp[0];
    doubleComplexSplit.imagp[fftSizeOver2] = 0.0;
    doubleComplexSplit.imagp[0] = 0.0;
    
    for (size_t i = 0; i <= fftSizeOver2; i++)
    {
        doubleComplexSplit.realp[i] *= 0.5;
        doubleComplexSplit.imagp[i] *= 0.5;
    }
    
    for (size_t i = fftSizeOver2 - 1; i > 0; --i)
    {
        doubleComplexSplit.realp[2 * fftSizeOver2 - i] = doubleComplexSplit.realp[i];
        doubleComplexSplit.imagp[2 * fftSizeOver2 - i] = -1 * doubleComplexSplit.imagp[i];
    }
    
    for (size_t i = 0; i < fftSize; i++)
    {
        real[i] = doubleComplexSplit.realp[i];
        imag[i] = doubleComplexSplit.imagp[i];
    }
}

//===========================================================
template class AccelerateFFT<float>;
template class AccelerateFFT<double>;

#endif