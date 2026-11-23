//=======================================================================
/** @file AccelerateFFT.h
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

#ifndef __AccelerateFFT__
#define __AccelerateFFT__

#ifdef USE_ACCELERATE_FFT

// this fixes a conflict with JUCE, remove if you don't need it
#define VIMAGE_H

#include <Accelerate/Accelerate.h>

//===========================================================
/** Performs the FFT using the Apple Accelerate Framework */
template <class T>
class AccelerateFFT
{
public:
    
    //===========================================================
    AccelerateFFT();
    ~AccelerateFFT();
    
    //===========================================================
    /** Sets the audio frame size to be used in the FFT */
    void setAudioFrameSize (int frameSize);
    
    /** Performs the FFT using Apple Accelerate FFT */
    void performFFT (T* buffer, T* real, T* imag);
    
private:
    
    size_t fftSize;
    size_t fftSizeOver2;
    size_t log2n;
    
    FFTSetup fftSetupFloat;
    FFTSetupD fftSetupDouble;
    COMPLEX_SPLIT complexSplit;
    DOUBLE_COMPLEX_SPLIT doubleComplexSplit;
    
    bool configured;
};

#endif

#endif /* __AccelerateFFT__ */
