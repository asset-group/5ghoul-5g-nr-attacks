//=======================================================================
/** @file WindowFunctions.cpp
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

#include "WindowFunctions.h"
#include <math.h>

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createWindow (int numSamples, WindowType windowType)
{
    if (windowType == HanningWindow)
    {
        std::vector<T> window = createHanningWindow (numSamples);
        return window;
    }
    else if (windowType == HammingWindow)
    {
        std::vector<T> window = createHammingWindow (numSamples);
        return window;
    }
    else if (windowType == BlackmanWindow)
    {
        std::vector<T> window = createBlackmanWindow (numSamples);
        return window;
    }
    else if (windowType == TukeyWindow)
    {
        std::vector<T> window = createTukeyWindow (numSamples);
        return window;
    }
    else // NOTE THIS COVERS THE RECTANGULAR WINDOW CASE AND ANY OTHER UNKNOWN TYPE
    {
        std::vector<T> window = createRectangularWindow (numSamples);
        return window;
    }
}

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createHanningWindow (int numSamples)
{
    std::vector<T> window (numSamples);
    
    T numSamplesMinus1 = (T) (numSamples - 1);		// the number of samples minus 1
    
    for (int i = 0; i < numSamples; i++)
    {
        window[i] = 0.5 * (1 - cos (2. * M_PI * (i / numSamplesMinus1)));
    }
    
    return window;
}

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createHammingWindow (int numSamples)
{
    std::vector<T> window (numSamples);
    
    T numSamplesMinus1 = (T) (numSamples - 1);		// the number of samples minus 1
    
    for (int i = 0; i < numSamples; i++)
        window[i] = 0.54 - (0.46 * cos (2 * M_PI * ((T)i / numSamplesMinus1)));
    
    return window;
}

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createBlackmanWindow (int numSamples)
{
    std::vector<T> window (numSamples);
    
    T numSamplesMinus1 = (T) (numSamples - 1);		// the number of samples minus 1
        
    for (int i = 0; i < numSamples; i++)
        window[i] = 0.42 - (0.5 * cos (2. * M_PI * ((T)i / numSamplesMinus1))) + (0.08 * cos (4. * M_PI * ((T)i / numSamplesMinus1)));
    
    return window;
}

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createTukeyWindow (int numSamples, T alpha)
{
    std::vector<T> window (numSamples);
    
    T numSamplesMinus1 = (T) (numSamples - 1);		// the number of samples minus 1
        
    T value = (double) (-1* ((numSamples / 2))) + 1;
    
    for (int i = 0; i < numSamples; i++)	// left taper
    {
        if ((value >= 0) && (value <= (alpha * (numSamplesMinus1 / 2))))
        {
            window[i] = 1.0;
        }
        else if ((value <= 0) && (value >= (-1 * alpha * (numSamplesMinus1 / 2))))
        {
            window[i] = 1.0;
        }
        else
        {
            window[i] = 0.5 * (1 + cos (M_PI * ((( 2 * value) / (alpha * numSamplesMinus1)) - 1)));
        }
        
        value += 1;
    }
    
    return window;
}

//===========================================================
template <class T>
std::vector<T> WindowFunctions<T>::createRectangularWindow (int numSamples)
{
    std::vector<T> window (numSamples);
    
    for (int i = 0; i < numSamples; i++)
        window[i] = 1.;
    
    return window;
}

//===========================================================
template class WindowFunctions<float>;
template class WindowFunctions<double>;
