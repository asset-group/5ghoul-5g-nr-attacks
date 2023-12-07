//=======================================================================
/** @file MFCC.h
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

#ifndef __GIST__MFCC__
#define __GIST__MFCC__

#define _USE_MATH_DEFINES
#include <vector>
#include <cmath>
#include <stddef.h>

//=======================================================================
/** Template class for calculating Mel Frequency Cepstral Coefficients
 * Instantiations of the class should be of either 'float' or
 * 'double' types and no others */
template <class T>
class MFCC
{
public:
    
    //=======================================================================
    /** Constructor */
    MFCC (int frameSize_, int samplingFrequency_);

    //=======================================================================
    /** Set the number of coefficients to calculate
     * @param numCoefficients_ the number of coefficients to calculate 
     */
    void setNumCoefficients (int numCoefficients_);

    /** Set the frame size - N.B. this will be twice the length of the magnitude spectrum passed to calculateMFCC()
     * @param frameSize_ the frame size
     */
    void setFrameSize (int frameSize_);

    /** Set the sampling frequency
     * @param samplingFrequency_ the sampling frequency in hz
     */
    void setSamplingFrequency (int samplingFrequency_);

    //=======================================================================
    /** Calculates the Mel Frequency Cepstral Coefficients from the magnitude spectrum of a signal. The result
     * is stored in the public vector MFCCs.
     * 
     * Note that the magnitude spectrum passed to the function is not the full mirrored magnitude spectrum, but 
     * only the first half. The frame size passed to the constructor should be twice the length of the magnitude spectrum.
     * @param magnitudeSpectrum the magnitude spectrum in vector format
     */
    void calculateMelFrequencyCepstralCoefficients (const std::vector<T>& magnitudeSpectrum);

    /** Calculates the magnitude spectrum on a Mel scale. The result is stored in
     * the public vector melSpectrum.
     */
    void calculateMelFrequencySpectrum (const std::vector<T>& magnitudeSpectrum);

    //=======================================================================
    /** a vector to hold the mel spectrum once it has been computed */
    std::vector<T> melSpectrum;
    
    /** a vector to hold the MFCCs once they have been computed */
    std::vector<T> MFCCs;
    
private:
    /** Initialises the parts of the algorithm dependent on frame size, sampling frequency
     * and the number of coefficients
     */
    void initialise();

    /** Calculates the discrete cosine transform (version 2) of an input signal, performing it in place
     * (i.e. the result is stored in the vector passed to the function)
     *
     * @param inputSignal a vector containing the input signal
     * @param numElements the number of elements in the input signal
     */
    void discreteCosineTransform (std::vector<T>& inputSignal, const std::size_t numElements);

    /** Calculates the triangular filters used in the algorithm. These will be different depending
     * upon the frame size, sampling frequency and number of coefficients and so should be re-calculated
     * should any of those parameters change.
     */
    void calculateMelFilterBank();

    /** Calculates mel from frequency
     * @param frequency the frequency in Hz
     * @returns the equivalent mel value
     */
    T frequencyToMel (T frequency);

    /** the sampling frequency in Hz */
    int samplingFrequency;

    /** the number of MFCCs to calculate */
    int numCoefficents;

    /** the audio frame size */
    int frameSize;

    /** the magnitude spectrum size (this will be half the frame size) */
    int magnitudeSpectrumSize;

    /** the minimum frequency to be used in the calculation of MFCCs */
    T minFrequency;

    /** the maximum frequency to be used in the calculation of MFCCs */
    T maxFrequency;

    /** a vector of vectors to hold the values of the triangular filters */
    std::vector<std::vector<T> > filterBank;
    std::vector<T> dctSignal;
};

#endif /* defined(__GIST__MFCC__) */
