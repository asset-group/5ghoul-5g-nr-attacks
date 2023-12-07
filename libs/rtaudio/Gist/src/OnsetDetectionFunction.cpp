//=======================================================================
/** @file OnsetDetectionFunction.cpp
 *  @brief Implementations of onset detection functions
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

#include "OnsetDetectionFunction.h"

//===========================================================
template <class T>
OnsetDetectionFunction<T>::OnsetDetectionFunction (int frameSize)
{
    // initialise buffers with the frame size
    setFrameSize (frameSize);
}

//===========================================================
template <class T>
void OnsetDetectionFunction<T>::setFrameSize (int frameSize)
{
    // resize the prev magnitude spectrum vector
    prevMagnitudeSpectrum_spectralDifference.resize (frameSize);
    prevMagnitudeSpectrum_spectralDifferenceHWR.resize (frameSize);
    prevPhaseSpectrum_complexSpectralDifference.resize (frameSize);
    prevPhaseSpectrum2_complexSpectralDifference.resize (frameSize);
    prevMagnitudeSpectrum_complexSpectralDifference.resize (frameSize);

    // fill it with zeros
    for (size_t i = 0; i < prevMagnitudeSpectrum_spectralDifference.size(); i++)
    {
        prevMagnitudeSpectrum_spectralDifference[i] = 0.0;
        prevMagnitudeSpectrum_spectralDifferenceHWR[i] = 0.0;
        prevPhaseSpectrum_complexSpectralDifference[i] = 0.0;
        prevPhaseSpectrum2_complexSpectralDifference[i] = 0.0;
        prevMagnitudeSpectrum_complexSpectralDifference[i] = 0.0;
    }

    prevEnergySum = 0;
}

//-----------------------------------------------------------
//-----------------------------------------------------------

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::energyDifference (const std::vector<T>& buffer)
{
    T sum;
    T difference;

    sum = 0; // initialise sum

    // sum the squares of the samples
    for (size_t i = 0; i < buffer.size(); i++)
        sum = sum + (buffer[i] * buffer[i]);

    difference = sum - prevEnergySum; // sample is first order difference in energy

    prevEnergySum = sum; // store energy value for next calculation

    if (difference > 0)
        return difference;
    else
        return 0.0;
}

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::spectralDifference (const std::vector<T>& magnitudeSpectrum)
{
    T sum = 0; // initialise sum to zero

    for (size_t i = 0; i < magnitudeSpectrum.size(); i++)
    {
        // calculate difference
        T diff = magnitudeSpectrum[i] - prevMagnitudeSpectrum_spectralDifference[i];

        // ensure all difference values are positive
        if (diff < 0)
        {
            diff = diff * -1;
        }

        // add difference to sum
        sum = sum + diff;

        // store the sample for next time
        prevMagnitudeSpectrum_spectralDifference[i] = magnitudeSpectrum[i];
    }

    return sum;
}

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::spectralDifferenceHWR (const std::vector<T>& magnitudeSpectrum)
{
    T sum = 0; // initialise sum to zero

    for (size_t i = 0; i < magnitudeSpectrum.size(); i++)
    {
        // calculate difference
        T diff = magnitudeSpectrum[i] - prevMagnitudeSpectrum_spectralDifferenceHWR[i];

        // only for positive changes
        if (diff > 0)
        {
            // add difference to sum
            sum = sum + diff;
        }

        // store the sample for next time
        prevMagnitudeSpectrum_spectralDifferenceHWR[i] = magnitudeSpectrum[i];
    }

    return sum;
}

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::complexSpectralDifference (const std::vector<T>& fftReal, const std::vector<T>& fftImag)
{
    T dev, pdev;
    T sum;
    T magDiff, phaseDiff;
    T value;
    T phaseVal;
    T magVal;

    sum = 0; // initialise sum to zero

    // compute phase values from fft output and sum deviations
    for (size_t i = 0; i < fftReal.size(); i++)
    {
        // calculate phase value
        phaseVal = atan2 (fftImag[i], fftReal[i]);

        // calculate magnitude value
        magVal = sqrt ((fftReal[i] * fftReal[i]) + (fftImag[i] * fftImag[i]));

        // phase deviation
        dev = phaseVal - (2 * prevPhaseSpectrum_complexSpectralDifference[i]) + prevPhaseSpectrum2_complexSpectralDifference[i];

        // wrap into [-pi,pi] range
        pdev = princarg (dev);

        // calculate magnitude difference (real part of Euclidean distance between complex frames)
        magDiff = magVal - prevMagnitudeSpectrum_complexSpectralDifference[i];

        // calculate phase difference (imaginary part of Euclidean distance between complex frames)
        phaseDiff = -magVal * sin (pdev);

        // square real and imaginary parts, sum and take square root
        value = sqrt ((magDiff * magDiff) + (phaseDiff * phaseDiff));

        // add to sum
        sum = sum + value;

        // store values for next calculation
        prevPhaseSpectrum2_complexSpectralDifference[i] = prevPhaseSpectrum_complexSpectralDifference[i];
        prevPhaseSpectrum_complexSpectralDifference[i] = phaseVal;
        prevMagnitudeSpectrum_complexSpectralDifference[i] = magVal;
    }

    return sum;
}

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::highFrequencyContent (const std::vector<T>& magnitudeSpectrum)
{
    T sum = 0;

    for (size_t i = 0; i < magnitudeSpectrum.size(); i++)
        sum += (magnitudeSpectrum[i] * ((T)(i + 1)));

    return sum;
}

//===========================================================
template <class T>
T OnsetDetectionFunction<T>::princarg (T phaseVal)
{
    // if phase value is less than or equal to -pi then add 2*pi
    while (phaseVal <= (-M_PI))
        phaseVal = phaseVal + (2 * M_PI);

    // if phase value is larger than pi, then subtract 2*pi
    while (phaseVal > M_PI)
        phaseVal = phaseVal - (2 * M_PI);

    return phaseVal;
}

//===========================================================
template class OnsetDetectionFunction<float>;
template class OnsetDetectionFunction<double>;
