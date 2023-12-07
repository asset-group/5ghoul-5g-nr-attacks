<div align="center"><img src="images/header.png"></div> 

Gist - An Audio Analysis Library
==================================

<!-- Version and License Badges -->
![Version](https://img.shields.io/badge/version-1.0.6-green.svg?style=flat-square) 
![License](https://img.shields.io/badge/license-GPL-blue.svg?style=flat-square) 
![Language](https://img.shields.io/badge/language-C++-yellow.svg?style=flat-square) 

Gist is a C++ based audio analysis library

Author
------

Gist is written and maintained by Adam Stark.

[http://www.adamstark.co.uk](http://www.adamstark.co.uk)

Usage
-----

Firstly, import the Gist header file:

	#include "Gist.h"
	
##### Instantiation	

Gist is a template class, so instantiate it with floating point precision:

	int frameSize = 512;
	int sampleRate = 44100;

	Gist<float> gist (frameSize, sampleRate);
	
Or with double precision:

	Gist<double> gist (frameSize, sampleRate);

We proceed with the documentation as if we were using floating point precision.

##### Process Audio Frames		

Once you have an audio frame, pass it to the Gist object. You can do this either as a STL vector:
	
	std::vector<float> audioFrame;
	
	// !
	// fill audio frame with samples here
	// !
	
	gist.processAudioFrame (audioFrame);
	
Or, as an array:

	float audioFrame[512];
	
	// !
	// fill audio frame with samples here
	// !
	
	gist.processAudioFrame (audioFrame, 512);
	
Now we can retrieve some audio features.
	
##### Core Time Domain Features
	
	// Root Mean Square (RMS)
	float rms = gist.rootMeanSquare();
	
	// Peak Energy
	float peakEnergy = gist.peakEnergy();
	
	// Zero Crossing rate
	float zcr = gist.zeroCrossingRate();
	
##### Core Frequency Domain Features
	
	// Spectral Centroid
	float specCent = gist.spectralCentroid();
	
    // Spectral Crest
    float specCrest = gist.spectralCrest();
    
    // Spectral Flatness
    float specFlat = gist.spectralFlatness();
    
    // Spectral Rolloff
    float specRolloff = gist.spectralRolloff();
    
    // Spectral Kurtosis
    float specKurtosis = gist.spectralKurtosis();

##### Onset Detection Functions
    
    // Energy difference
    float ed = gist.energyDifference();
    
    // Spectral difference
    float sd = gist.spectralDifference();
    
    // Spectral difference (half-wave rectified)
    float sd_hwr = gist.spectralDifferenceHWR();
    
    // Complex Spectral Difference
    float csd = gist.complexSpectralDifference();
    
    // High Frequency Content
    float hfc = gist.highFrequencyContent();
    
##### FFT Magnitude Spectrum

	// FFT Magnitude Spectrum
	const std::vector<float>& magSpec = gist.getMagnitudeSpectrum();
	
##### Pitch

	// Pitch Estimation
	float pitch = gist.pitch();

##### Mel-frequency Representations

	// Mel-frequency Spectrum
	const std::vector<float>& melSpec = gist.getMelFrequencySpectrum();
	
	// MFCCs
	const std::vector<float>& mfcc = gist.getMelFrequencyCepstralCoefficients();
	
		
Version History
---------------

=== 1.0.6 === (21st October 2020)

* CMake support
* Python module moved to Python 3

=== 1.0.5 === (29th February 2020)

* Fix for compilation error on Windows
* Fixed lots of warnings
* Update to code style

=== 1.0.4 === (22nd January 2017)

* Small changes to the interface for MFCCs and Mel Spectrum
* Many internal improvements
* Added window functions

=== 1.0.3 === (17th June 2016)

* Added a Python module
* Added ability to set and get sampling frequency
* Bug fixes and implementation improvements

=== 1.0.2 === (24th April 2016)

* Added the option of using Apple Accelerate FFT
* Added two new features: Spectral Rolloff and Spectral Kurtosis
* Small usability and code style tweaks

=== 1.0.1 === (26th June 2014)

* Added the option of using Kiss FFT instead of FFTW

=== 1.0.0 === (22nd June 2014)

* The first version of Gist

Dependencies
------------

The Gist library depends on one of the following FFT libraries:

* [FFTW](http://fftw.org) 

You will need to install this yourself, link projects using -lfftw3 and use the flag -DUSE_FFTW

* [Kiss FFT](http://kissfft.sourceforge.net/) - included with project

This is included with the project. To use Kiss FFT, add the flag -DUSE_KISS_FFT

* [Apple Accelerate FFT](https://developer.apple.com/library/ios/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html)

To use Accelerate FFT, add the flag -DUSE_ACCELERATE_FFT

License
-------

Copyright (c) 2014 Adam Stark

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.



