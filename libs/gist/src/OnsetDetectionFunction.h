//=======================================================================
/** @file OnsetDetectionFunction.h
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

#ifndef __GIST__ONSETDETECTIONFUNCTION__
#define __GIST__ONSETDETECTIONFUNCTION__

#define _USE_MATH_DEFINES
#include <vector>
#include <cmath>

/** template class for calculating onset detection functions
 * Instantiations of the class should be of either 'float' or 
 * 'double' types and no others */
template <class T>
class OnsetDetectionFunction
{
public:
    //===========================================================
    /** constructor */
    OnsetDetectionFunction (int frameSize);

    //===========================================================
    /** Sets the frame size of internal buffers. Assumes all magnitude
     * spectra are passed as the first half (i.e. not mirrored)
     * @param frameSize the frame size
     */
    void setFrameSize (int frameSize);

    //===========================================================
    /** calculates the energy difference onset detection function
     * @param buffer the time domain audio frame containing audio samples
     * @returns the energy difference onset detection function sample for the frame
     */
    T energyDifference (const std::vector<T>& buffer);

    //===========================================================
    /** calculates the spectral difference between the current magnitude
     * spectrum and the previous magnitude spectrum
     * @param magnitudeSpectrum a vector containing the magnitude spectrum
     * @returns the spectral difference onset detection function sample
     */
    T spectralDifference (const std::vector<T>& magnitudeSpectrum);

    //===========================================================
    /** calculates the half wave rectified spectral difference between the 
     * current magnitude spectrum and the previous magnitude spectrum
     * @param magnitudeSpectrum a vector containing the magnitude spectrum
     * @returns the HWR spectral difference onset detection function sample
     */
    T spectralDifferenceHWR (const std::vector<T>& magnitudeSpectrum);

    //===========================================================
    /** calculates the complex spectral difference from the real and imaginary parts 
     * of the FFT
     * @param fftReal a vector containing the real part of the FFT
     * @param fftImag a vector containing the imaginary part of the FFT
     * @returns the complex spectral difference onset detection function sample
     */
    T complexSpectralDifference (const std::vector<T>& fftReal, const std::vector<T>& fftImag);

    //===========================================================
    /** calculates the high frequency content onset detection function from
     * the magnitude spectrum
     * @param magnitudeSpectrum a vector containing the magnitude spectrum
     * @returns the high frequency content onset detection function sample
     */
    T highFrequencyContent (const std::vector<T>& magnitudeSpectrum);

private:
    /** maps phasein into the [-pi:pi] range */
    T princarg (T phaseVal);

    //===========================================================
    /** holds the previous energy sum for the energy difference onset detection function */
    T prevEnergySum;

    /** a vector containing the previous magnitude spectrum passed to the
     last spectral difference call */
    std::vector<T> prevMagnitudeSpectrum_spectralDifference;

    /** a vector containing the previous magnitude spectrum passed to the
     last spectral difference (half wave rectified) call */
    std::vector<T> prevMagnitudeSpectrum_spectralDifferenceHWR;

    /** a vector containing the previous phase spectrum passed to the
     last complex spectral difference call */
    std::vector<T> prevPhaseSpectrum_complexSpectralDifference;

    /** a vector containing the second previous phase spectrum passed to the
     last complex spectral difference call */
    std::vector<T> prevPhaseSpectrum2_complexSpectralDifference;

    /** a vector containing the previous magnitude spectrum passed to the
     last complex spectral difference call */
    std::vector<T> prevMagnitudeSpectrum_complexSpectralDifference;
};

#endif
