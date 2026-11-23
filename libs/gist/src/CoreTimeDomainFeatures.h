//=======================================================================
/** @file CoreTimeDomainFeatures.h
 *  @brief Implementations of common time domain audio features
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

#ifndef __GIST__CORETIMEDOMAINFEATURES__
#define __GIST__CORETIMEDOMAINFEATURES__

#include <vector>
#include <math.h>

/** template class for calculating common time domain
 * audio features. Instantiations of the class should be
 * of either 'float' or 'double' types and no others */
template <class T>
class CoreTimeDomainFeatures
{
public:
    /** constructor */
    CoreTimeDomainFeatures();

    //===========================================================
    /** calculates the Root Mean Square (RMS) of an audio buffer
     * in vector format
     * @param buffer a time domain buffer containing audio samples
     * @returns the RMS value
     */
    T rootMeanSquare (const std::vector<T>& buffer);

    //===========================================================
    /** calculates the peak energy (max absolute value) in a time
     * domain audio signal buffer in vector format
     * @param buffer a time domain buffer containing audio samples
     * @returns the peak energy value
     */
    T peakEnergy (const std::vector<T>& buffer);

    //===========================================================
    /** calculates the zero crossing rate of a time domain audio signal buffer
     * @param buffer a time domain buffer containing audio samples
     * @returns the zero crossing rate
     */
    T zeroCrossingRate (const std::vector<T>& buffer);
};

#endif
