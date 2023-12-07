include_directories (doctest)
include_directories (test-signals)
include_directories (${Gist_SOURCE_DIR}/src)
include_directories (${Gist_SOURCE_DIR}/libs/kiss_fft130)

add_executable (Tests 
    main.cpp 
    ${Gist_SOURCE_DIR}/libs/kiss_fft130/kiss_fft.c
    test-signals/Test_Signals.cpp 
    Test_CoreFrequencyDomainFeatures.cpp
    Test_CoreTimeDomainFeatures.cpp
    Test_Gist.cpp
    Test_MFCC.cpp
    Test_OnsetDetectionFunction.cpp
    Test_Pitch.cpp
    )

target_link_libraries (Tests Gist)
target_compile_features (Tests PRIVATE cxx_std_17)
add_test (NAME Tests COMMAND Tests)