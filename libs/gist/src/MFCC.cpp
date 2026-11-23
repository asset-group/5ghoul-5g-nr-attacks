//=======================================================================
/** @file MFCC.cpp
 *  @brief Calculates Mel Frequency Cepstral Coefficients
 *  @author Adam Stark
 *  @copyright Copyright (C) 2014  Adam Stark
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

#include "MFCC.h"
#include <cfloat>
#include <assert.h>

//==================================================================
template <class T>
MFCC<T>::MFCC (int frameSize_, int samplingFrequency_)
{
    numCoefficents = 13;
    frameSize = frameSize_;
    samplingFrequency = samplingFrequency_;

    initialise();
}

//==================================================================
template <class T>
void MFCC<T>::setNumCoefficients (int numCoefficients_)
{
    numCoefficents = numCoefficients_;
    initialise();
}

//==================================================================
template <class T>
void MFCC<T>::setFrameSize (int frameSize_)
{
    frameSize = frameSize_;
    initialise();
}

//==================================================================
template <class T>
void MFCC<T>::setSamplingFrequency (int samplingFrequency_)
{
    samplingFrequency = samplingFrequency_;
    initialise();
}

//==================================================================
template <class T>
void MFCC<T>::calculateMelFrequencyCepstralCoefficients (const std::vector<T>& magnitudeSpectrum)
{
    calculateMelFrequencySpectrum (magnitudeSpectrum);
    
    for (size_t i = 0; i < melSpectrum.size(); i++)
        MFCCs[i] = log (melSpectrum[i] + (T)FLT_MIN);

    discreteCosineTransform (MFCCs, MFCCs.size());
}

//==================================================================
template <class T>
void MFCC<T>::calculateMelFrequencySpectrum (const std::vector<T>& magnitudeSpectrum)
{
    for (int i = 0; i < numCoefficents; i++)
    {
        double coeff = 0;
        
        for (size_t j = 0; j < magnitudeSpectrum.size(); j++)
            coeff += (T)((magnitudeSpectrum[j] * magnitudeSpectrum[j]) * filterBank[i][j]);
        
        melSpectrum[i] = coeff;
    }
}

//==================================================================
template <class T>
void MFCC<T>::initialise()
{
    magnitudeSpectrumSize = frameSize / 2;
    minFrequency = 0;
    maxFrequency = samplingFrequency / 2;

    melSpectrum.resize (numCoefficents);
    MFCCs.resize (numCoefficents);
    dctSignal.resize (numCoefficents);
    
    calculateMelFilterBank();
}

//==================================================================
template <class T>
void MFCC<T>::discreteCosineTransform (std::vector<T>& inputSignal, const std::size_t numElements)
{
    // the input signal must have the number of elements specified in the numElements variable
    assert (inputSignal.size() == numElements);
    
    // this should already be the case - sanity check
    assert (dctSignal.size() == numElements);
        
    for (size_t i = 0; i < numElements; i++)
        dctSignal[i] = inputSignal[i];
    
    T N = (T)numElements;
    T piOverN = M_PI / N;

    for (size_t k = 0; k < numElements; k++)
    {
        T sum = 0;
        T kVal = (T)k;

        for (size_t n = 0; n < numElements; n++)
        {
            T tmp = piOverN * (((T)n) + 0.5) * kVal;
            sum += dctSignal[n] * cos (tmp);
        }

        inputSignal[k] = (T)(2 * sum);
    }
}

//==================================================================
template <class T>
void MFCC<T>::calculateMelFilterBank()
{
    int maxMel = floor (frequencyToMel (maxFrequency));
    int minMel = floor (frequencyToMel (minFrequency));

    filterBank.resize (numCoefficents);

    for (int i = 0; i < numCoefficents; i++)
    {
        filterBank[i].resize (magnitudeSpectrumSize);

        for (int j = 0; j < magnitudeSpectrumSize; j++)
            filterBank[i][j] = 0.0;
    }

    std::vector<int> centreIndices;

    for (int i = 0; i < numCoefficents + 2; i++)
    {
        double f = i * (maxMel - minMel) / (numCoefficents + 1) + minMel;

        double tmp = log (1 + 1000.0 / 700.0) / 1000.0;
        tmp = (exp (f * tmp) - 1) / (samplingFrequency / 2);
        tmp = 0.5 + 700 * ((double)magnitudeSpectrumSize) * tmp;
        tmp = floor (tmp);

        int centreIndex = (int)tmp;
        centreIndices.push_back (centreIndex);
    }

    for (int i = 0; i < numCoefficents; i++)
    {
        int filterBeginIndex = centreIndices[i];
        int filterCenterIndex = centreIndices[i + 1];
        int filterEndIndex = centreIndices[i + 2];

        T triangleRangeUp = (T)(filterCenterIndex - filterBeginIndex);
        T triangleRangeDown = (T)(filterEndIndex - filterCenterIndex);

        // upward slope
        for (int k = filterBeginIndex; k < filterCenterIndex; k++)
            filterBank[i][k] = ((T)(k - filterBeginIndex)) / triangleRangeUp;

        // downwards slope
        for (int k = filterCenterIndex; k < filterEndIndex; k++)
            filterBank[i][k] = ((T)(filterEndIndex - k)) / triangleRangeDown;
    }
}

//==================================================================
template <class T>
T MFCC<T>::frequencyToMel (T frequency)
{
    return int(1127) * log (1 + (frequency / 700.0));
}

//===========================================================
template class MFCC<float>;
template class MFCC<double>;
