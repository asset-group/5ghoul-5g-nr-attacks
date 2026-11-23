
add_library (
    Gist STATIC
    ${PROJECT_SOURCE_DIR}/libs/kiss_fft130/kiss_fft.c
    AccelerateFFT.cpp
    AccelerateFFT.h
    CoreFrequencyDomainFeatures.cpp
    CoreFrequencyDomainFeatures.h
    CoreTimeDomainFeatures.cpp
    CoreTimeDomainFeatures.h
    Gist.cpp
    Gist.h
    MFCC.cpp
    MFCC.h
    OnsetDetectionFunction.cpp
    OnsetDetectionFunction.h
    WindowFunctions.cpp
    WindowFunctions.h
    Yin.cpp
    Yin.h
)

target_include_directories(Gist PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(Gist PUBLIC ${PROJECT_SOURCE_DIR}/libs/kiss_fft130)

source_group (Source src)

target_compile_definitions (Gist PUBLIC -DUSE_KISS_FFT)

export(TARGETS Gist NAMESPACE Gist::
FILE ${PROJECT_BINARY_DIR}/Gist-targets.cmake)
