//=======================================================================
/** @file Yin.h
 *  @brief Implementation of the YIN pitch detection algorithm (de Cheveigné and Kawahara,2002)
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

#ifndef __GIST__YIN__
#define __GIST__YIN__

#include <vector>
#include <cmath>

//===========================================================
/** template class for the pitch detection algorithm Yin.
 * Instantiations of the class should be of either 'float' or
 * 'double' types and no others */
template <class T>
class Yin
{
    
public:
    
    //===========================================================
    /** constructor
     * @param samplingFrequency the sampling frequency
     */
    Yin (int samplingFrequency);
    
    //===========================================================
    /** sets the sampling frequency used to calculate pitch values
     * @param samplingFrequency the sampling frequency
     */
    void setSamplingFrequency (int samplingFrequency);
    
    /** sets the maximum frequency that the algorithm will return
     * @param maxFreq the maximum frequency
     */
    void setMaxFrequency (T maxFreq);
    
    //===========================================================
    /** @returns the maximum frequency that the algorithm will return */
    T getMaxFrequency()
    {
        return ((T) fs) / ((T) minPeriod);
    }
    
    //===========================================================
    /** calculates the pitch of the audio frame passed to it
     * @param frame an audio frame stored in a vector
     * @returns the estimated pitch in Hz
     */
    T pitchYin (const std::vector<T>& frame);
        
private:
    
    //===========================================================
    /** converts periods to pitch in Hz
     * @param period the period in audio samples
     * @returns the pitch in Hz
     */
    T periodToPitch (T period);

    /** this method searches the previous period estimate for a 
     * minimum and if it finds one, it is used, for the sake of consistency, 
     * even if it is not the optimal choice 
     * @param delta the cumulative mean normalised difference function
     * @returns the period found if a minimum is found, or -1 if not
     */
    long searchForOtherRecentMinima (const std::vector<T>& delta);
    
    /** interpolates a period estimate using parabolic interpolation
     * @param period the period estimate
     * @param y1 the value of the cumulative mean normalised difference function at (period-1)
     * @param y2 the value of the cumulative mean normalised difference function at (period)
     * @param y3 the value of the cumulative mean normalised difference function at (period+1)
     * @returns the interpolated period
     */
    T parabolicInterpolation (unsigned long period,T y1,T y2,T y3);
    
    /** calculates the period candidate from the cumulative mean normalised difference function 
     * @param delta the cumulative mean normalised difference function
     * @returns the period estimate
     */
    unsigned long getPeriodCandidate (const std::vector<T>& delta);
    
    /** this calculates steps 1, 2 and 3 of the Yin algorithm as set out in
     * the paper (de Cheveigné and Kawahara,2002).
     * @param frame a vector containing the audio frame to be procesed
     */
    void cumulativeMeanNormalisedDifferenceFunction (const std::vector<T>& frame);
    
	T round (T val)
	{
		return floor(val + 0.5);
	}

    /** the previous period estimate found by the algorithm last time it was called - initially set to 1.0 */
    T prevPeriodEstimate;
    
    /** the sampling frequency */
    int fs;
    
    /** the minimum period the algorithm will look for. this is set indirectly by setMaxFrequency() */
    int minPeriod;
    
    std::vector<T> delta;
};

#endif
