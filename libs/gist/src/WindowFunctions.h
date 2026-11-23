//=======================================================================
/** @file WindowFunctions.h
 *  @brief A Collection of Window Functions
 *  @author Adam Stark
 *  @copyright Copyright (C) 2016  Adam Stark
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

#ifndef __WindowFunctions__
#define __WindowFunctions__

#define _USE_MATH_DEFINES
#include <vector>
#include <cmath>

//=======================================================================
/** A type indicator for different windows */
enum WindowType
{
    RectangularWindow,
    HanningWindow,
    HammingWindow,
    BlackmanWindow,
    TukeyWindow
};

//=======================================================================
/** A collection of different window functions */
template <class T>
class WindowFunctions
{
public:
    
    //=======================================================================
    /** @Returns a window with a specified type */
    static std::vector<T> createWindow (int numSamples, WindowType windowType);
    
    //=======================================================================
    /** @Returns a Hanning window */
    static std::vector<T> createHanningWindow (int numSamples);
    
    /** @Returns a Hamming window */
    static std::vector<T> createHammingWindow (int numSamples);
    
    /** @Returns a Blackman window */
    static std::vector<T> createBlackmanWindow (int numSamples);
    
    /** @Returns a Tukey window */
    static std::vector<T> createTukeyWindow (int numSamples, T alpha = 0.5);
    
    /** @Returns a Rectangular window */
    static std::vector<T> createRectangularWindow (int numSamples);
};


#endif /* WindowFunctions_h */
