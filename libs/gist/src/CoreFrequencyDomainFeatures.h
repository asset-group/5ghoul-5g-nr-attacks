//=======================================================================
/** @file CoreFrequencyDomainFeatures.h
 *  @brief Implementations of common frequency domain audio features
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

#ifndef __GIST__COREFREQUENCYDOMAINFEATURES__
#define __GIST__COREFREQUENCYDOMAINFEATURES__

#include <vector>
#include <numeric>
#include <math.h>

/** template class for calculating common frequency domain
 * audio features. Instantiations of the class should be
 * of either 'float' or 'double' types and no others */
template <class T>
class CoreFrequencyDomainFeatures
{
public:
    /** constructor */
    CoreFrequencyDomainFeatures();

    //===========================================================
    /** calculates the spectral centroid given the first half of the magnitude spectrum
     of an audio signal. Do not pass the whole (i.e. mirrored) magnitude spectrum into
     this function or you will always get the middle index as the spectral centroid 
     @param magnitudeSpectrum the first half of the magnitude spectrum (i.e. not mirrored)
     @returns the spectral centroid as an index value
     */
    T spectralCentroid (const std::vector<T>& magnitudeSpectrum);

    //===========================================================
    /** calculates the spectral flatness given the first half of the magnitude spectrum
     of an audio signal.
     @param magnitudeSpectrum the first half of the magnitude spectrum (i.e. not mirrored)
     @returns the spectral flatness
     */
    T spectralFlatness (const std::vector<T>& magnitudeSpectrum);

    //===========================================================
    /** calculates the spectral crest given the first half of the magnitude spectrum
     of an audio signal.
     @param magnitudeSpectrum the first half of the magnitude spectrum (i.e. not mirrored)
     @returns the spectral crest
     */
    T spectralCrest (const std::vector<T>& magnitudeSpectrum);
    
    //===========================================================
    /** calculates the spectral rolloff given the first half of the magnitude spectrum
     of an audio signal.
     @param magnitudeSpectrum the first half of the magnitude spectrum (i.e. not mirrored)
     @param percentile the rolloff threshold
     @returns the spectral rolloff
     */
    T spectralRolloff (const std::vector<T>& magnitudeSpectrum, T percentile = 0.85);
    
    //===========================================================
    /** calculates the spectral kurtosis given the first half of the magnitude spectrum
     of an audio signal.
     @param magnitudeSpectrum the first half of the magnitude spectrum (i.e. not mirrored)
     @returns the spectral kurtosis
     */
    T spectralKurtosis (const std::vector<T>& magnitudeSpectrum);
    
    
};

#endif
