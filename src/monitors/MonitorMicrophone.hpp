#pragma once

#ifndef __MONITORMICROPHONE__
#define __MONITORMICROPHONE__

#include <functional>
#include <pthread.h>
#include <thread>
#include <string>
#include <src/Machine.hpp>
#include <libs/rtaudio/RtAudio.h>
#include <libs/rtaudio/Gist/src/Gist.h>
#include <libs/mv_average.hpp>

using namespace std;

class MonitorMicrophone
{
private:
    const char *TAG = "[Monitor] ";

    vector<string> *MagicWords;
    bool *enable = nullptr;
    function<void(string)> UserCallback = nullptr;
    function<void(bool)> UserCrashCallback = nullptr;
    bool running = false;
    int _device_id;
    double _detection_sensitivity;

    Gist<float> *gist;
    mv_average<100> avg_mic_rms;
    mv_average<10> avg_fast_rms;
    uint32_t debouncer = 0;
    unsigned int bufferFrames;

    RtAudio adc;
    RtAudio::DeviceInfo info;
    RtAudio::StreamParameters iParams;
    RtAudio::StreamOptions options;

    static int sound_detection(void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
                               double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data)
    {
        MonitorMicrophone *cls = (MonitorMicrophone *)data;

        cls->gist->processAudioFrame((float *)inputBuffer, nBufferFrames);

        // Get rms limits from previous slow rms array
        float prev_deviation = (float)cls->avg_mic_rms.avg_deviation();
        float max_rms = (float)cls->avg_mic_rms.mean() + prev_deviation;
        float rms_threshold = (max_rms * cls->_detection_sensitivity) + max_rms;

        // Get current rms
        float c_rms = cls->gist->rootMeanSquare() * 100000.0;
        // Add to fast rms array
        cls->avg_fast_rms.add_sample(c_rms);
        // Compare if the fast rms array is greater than max rms threshold
        if (cls->avg_fast_rms.mean() > rms_threshold)
        {
            if (!cls->debouncer)
            {
                // Audio trigger
                cls->UserCrashCallback(false);
                std::string info = "[Trigger] Level=" + std::to_string((uint32_t)cls->avg_fast_rms.mean()) +
                                   ", Max Level=" + std::to_string((uint32_t)rms_threshold) +
                                   ", Deviation=" + std::to_string((uint32_t)prev_deviation);
                cls->UserCallback(info);
            }

            if (cls->debouncer < 100)
                cls->debouncer = 100;
        }

        if (cls->debouncer)
            cls->debouncer--;
        // Add current rms to slow rms array
        cls->avg_mic_rms.add_sample(c_rms);
        // std::cout << "Level=" << cls->avg_mic_rms.mean() << std::endl;
        // std::cout << "Max Level=" << rms_threshold << std::endl;
        // std::cout << "Prev Deviation=" << (uint32_t)prev_deviation << std::endl;
        // std::cout << "Deviation=" << cls->avg_mic_rms.avg_deviation() << std::endl;
        // std::cout << "Debouncer=" << cls->debouncer << std::endl;
        // std::cout << "-----------------" << std::endl;

        return 0;
    }

public:
    void printStatus()
    {

        if (info.name.size() < 1)
        {
            GL1R(TAG, "Device id ", iParams.deviceId, " Invalid");
            return;
        }

        GL1G(TAG, "Selected Device Name = ", info.name, ", ID = ", iParams.deviceId);
    }

    bool IsOpen()
    {
        return adc.isStreamRunning();
    }

    ~MonitorMicrophone()
    {
        running = false;
        stop();
    }

    bool init()
    {
        unsigned int devices = adc.getDeviceCount();
        if (devices < 1)
        {
            GL1R(TAG, "No audio devices found!");
            return false;
        }

        GL1G(TAG, "Found ", devices, " audio device(s) ...");
        for (unsigned int i = 0; i < devices; i++)
        {
            info = adc.getDeviceInfo(i);
            if (info.probed)
            {
                GL1Y(TAG, "Audio -> Device Name = ", info.name, ", Device ID = ", i);
            }
        }

        if (_device_id == -1)
            iParams.deviceId = adc.getDefaultInputDevice();
        else
            iParams.deviceId = _device_id;

        info = adc.getDeviceInfo(iParams.deviceId);

        if (info.name.size() < 1)
        {
            return false;
        }

        // Configure channel with default parameters
        adc.showWarnings(true);
        iParams.nChannels = 1;
        iParams.firstChannel = 0;
        options.flags = RTAUDIO_HOG_DEVICE | RTAUDIO_MINIMIZE_LATENCY;

        gist = new Gist<float>(256, 44100);
        if (gist == nullptr)
            return false;

        try
        {
            bufferFrames = 256;
            adc.openStream(NULL, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &MonitorMicrophone::sound_detection, this);
        }
        catch (RtAudioError &e)
        {
            GL1R(TAG, e.getMessage());
            return false;
        }

        debouncer = 200;
        try
        {

            adc.startStream();
        }
        catch (RtAudioError &e)
        {
            GL1R(TAG, e.getMessage());
            return false;
        }

        return true;
    }

    bool stop()
    {
        if (adc.isStreamOpen())
        {
            adc.closeStream();
            return true;
        }
        return false;
    }

    bool init(Config &conf)
    {
        return init(conf.monitor.microphone.microphone_device_id, conf.monitor.microphone.microphone_detection_sensitivity);
    }

    bool init(int device_id, double detection_sensitivity)
    {
        _device_id = device_id;
        _detection_sensitivity = detection_sensitivity;

        return init();
    }

    void SetMagicWords(vector<string> &magicwords)
    {
        MagicWords = &magicwords;
    }

    void SetCallback(function<void(string)> callback)
    {
        UserCallback = callback;
    }

    void SetCrashCallback(function<void(bool)> callback)
    {
        UserCrashCallback = callback;
    }
};

#endif
