/******************************************/
/*
  record.cpp
  by Gary P. Scavone, 2007

  This program records audio from a device and writes it to a
  header-less binary file.  Use the 'playraw', with the same
  parameters and format settings, to playback the audio.
*/
/******************************************/

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include "Gist/src/Gist.h"
#include "mv_average.hpp"


#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#define FRAME_SIZE 256
#define MAX_RMS_MULTIPLIER 3.0
#define MIN_RMS_MULTIPLIER 1.2


typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32


struct InputData {
  MY_TYPE* buffer;
  unsigned long bufferBytes;
  unsigned long totalFrames;
  unsigned long frameCounter;
  unsigned int channels;
};

Gist<float> *gist;
mv_average<100> avg_mic_rms;
mv_average<10> avg_fast_rms;
uint32_t debouncer = 0;

void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: record N fs <duration> <device> <channelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    duration = optional time in seconds to record (default = 2.0),\n";
  std::cout << "    device = optional device to use (default = 0),\n";
  std::cout << "    and channelOffset = an optional channel offset on the device (default = 0).\n\n";
  exit( 0 );
}

// Interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data )
{
  InputData *iData = (InputData *) data;

  // Simply copy the data to our allocated buffer.
  unsigned int frames = nBufferFrames;
  if ( iData->frameCounter + nBufferFrames > iData->totalFrames ) {
    frames = iData->totalFrames - iData->frameCounter;
    iData->bufferBytes = frames * iData->channels * sizeof( MY_TYPE );
  }

  unsigned long offset = iData->frameCounter * iData->channels;
  memcpy( iData->buffer+offset, inputBuffer, iData->bufferBytes );
  iData->frameCounter += frames;

  // std::cout << nBufferFrames << std::endl;
  if(gist){
    gist->processAudioFrame ((float *)inputBuffer, FRAME_SIZE);

    // Get rms limits from previous slow rms array
    float prev_deviation = (float)avg_mic_rms.avg_deviation();
    float rms_threshold = (prev_deviation * 1.5) + (float)avg_mic_rms.mean();
    // Get current rms
    float c_rms = gist->rootMeanSquare() * 100000.0;
    // Add to fast rms array
    avg_fast_rms.add_sample(c_rms);
    // Compare if the fast rms array is greater than max rms threshold
    if(avg_fast_rms.mean() > rms_threshold){
      if(!debouncer)
        std::cout << "CRASH!!!!!!!!!!!" << std::endl;
      if (debouncer < 100);
        debouncer = 100;
    }

    if(debouncer)
      debouncer--;
    // Add current rms to slow rms array
    avg_mic_rms.add_sample(c_rms);
    std::cout << "Level=" << avg_mic_rms.mean() << std::endl;
    std::cout << "Max Level=" << rms_threshold << std::endl;
    std::cout << "Prev Deviation=" << (uint32_t)prev_deviation << std::endl;
    std::cout << "Deviation=" << avg_mic_rms.avg_deviation() << std::endl;
    std::cout << "Debouncer=" << debouncer << std::endl;

    std::cout << "-----------------" << std::endl;
  }

  if ( iData->frameCounter >= iData->totalFrames ) return 2;
  return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int channels, fs, bufferFrames, device = 0, offset = 0;
  double time = 2.0;
  FILE *fd;

  // minimal command-line checking
  if ( argc < 3 || argc > 6 ) usage();

  RtAudio adc;
  RtAudio::DeviceInfo info;

  unsigned int devices = adc.getDeviceCount();

  if ( devices < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  std::cout << "\nFound " << devices << " audio device(s) ...\n";

  for (unsigned int i=0; i<devices; i++) {
    info = adc.getDeviceInfo(i);
    if ( info.probed  ) {
      std::cout << "\n-->Device Name = " << info.name << '\n';
      std::cout << "-->Device ID = " << i << '\n';
    }
  }

  channels = (unsigned int) atoi( argv[1] );
  fs = (unsigned int) atoi( argv[2] );
  if ( argc > 3 )
    time = (double) atof( argv[3] );
  if ( argc > 4 )
    device = (unsigned int) atoi( argv[4] );
  if ( argc > 5 )
    offset = (unsigned int) atoi( argv[5] );

  // Let RtAudio print messages to stderr.
  adc.showWarnings( true );


  

  // Set our stream parameters for input only.
  bufferFrames = FRAME_SIZE;
  RtAudio::StreamParameters iParams;
  if ( device == 0 )
    iParams.deviceId = adc.getDefaultInputDevice();
  else
    iParams.deviceId = device;

  info = adc.getDeviceInfo(iParams.deviceId);
  std::cout << "\n------------------------------------------------------" << std::endl;
  std::cout << "Selected default device = " << info.name << '\n';
  std::cout << "Device ID = " << iParams.deviceId << '\n';
  std::cout << "------------------------------------------------------" << std::endl;

  iParams.nChannels = 1;
  iParams.firstChannel = offset;



  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_HOG_DEVICE | RTAUDIO_MINIMIZE_LATENCY;

  InputData data;
  data.buffer = 0;

  gist = new Gist<float>(FRAME_SIZE, fs);

  try {
    adc.openStream( NULL, &iParams, FORMAT, fs, &bufferFrames, &input, (void *)&data );
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

  data.bufferBytes = bufferFrames * channels * sizeof( MY_TYPE );
  data.totalFrames = (unsigned long) (fs * time);
  data.frameCounter = 0;
  data.channels = channels;
  unsigned long totalBytes;
  totalBytes = data.totalFrames * channels * sizeof( MY_TYPE );

  // Allocate the entire data buffer before starting stream.
  data.buffer = (MY_TYPE*) malloc( totalBytes );
  if ( data.buffer == 0 ) {
    std::cout << "Memory allocation error ... quitting!\n";
    goto cleanup;
  }

  try {
    debouncer = 1024;
    adc.startStream();
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

  std::cout << "\nRecording for " << time << " seconds ... writing file 'record.raw' (buffer frames = " << bufferFrames << ")." << std::endl;
  while ( adc.isStreamRunning() ) {
    SLEEP( 100 ); // wake every 100 ms to check if we're done
  }

  // Now write the entire data to the file.
  fd = fopen( "record.raw", "wb" );
  fwrite( data.buffer, sizeof( MY_TYPE ), data.totalFrames * channels, fd );
  fclose( fd );

 cleanup:
  if ( adc.isStreamOpen() ) adc.closeStream();
  if ( data.buffer ) free( data.buffer );
  if ( gist ) free( gist );

  return 0;
}
