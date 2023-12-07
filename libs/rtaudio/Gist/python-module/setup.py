# setup.py
# build command : python setup.py build build_ext --inplace
from numpy.distutils.core import setup, Extension
import os, numpy
import sys

if not sys.version_info.major == 3 and sys.version_info.minor >= 6:
      print ("")
      print ("Python Version Error")
      print ("")
      print ("This script must be run using Python 3.6 or later.")
      print ("You are trying to run it using Python " + str (sys.version_info.major) + "." + str (sys.version_info.minor))
      print ("")
      exit()

name = 'gist'
sources = [
'GistPythonModule.cpp',
'../src/Gist.cpp',
'../src/core/CoreFrequencyDomainFeatures.cpp',
'../src/core/CoreTimeDomainFeatures.cpp',
'../src/mfcc/MFCC.cpp',
'../src/onset-detection-functions/OnsetDetectionFunction.cpp',
'../src/pitch/Yin.cpp',
'../src/fft/WindowFunctions.cpp'
]

include_dirs = [
                numpy.get_include(),'/usr/local/include'
                ]

setup( name = 'Gist',
      include_dirs = include_dirs,
      ext_modules = [Extension(name, sources,libraries = ['fftw3'],library_dirs = ['/usr/local/lib'],define_macros=[
                         ('USE_FFTW', None)],)]
      )