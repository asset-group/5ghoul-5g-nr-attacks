#pragma once

#ifndef __GUICOMMON__
#define __GUICOMMON__

#include <libs/veque.hpp>

// utility structure for realtime plot
struct ScrollingBuffer {
    int MaxSize;
    int Idx;
    veque::veque<float> DataX;
    veque::veque<float> DataY;

    ScrollingBuffer(int max_size = 2000)
    {
        MaxSize = max_size;
        DataX.resize(MaxSize);
        DataY.resize(MaxSize);
        Idx = 0;
        for (size_t i = 0; i < MaxSize; i++) {
            DataX[i] = (float)i;
        }
    }

    void AddPoint(float y)
    {
        if (Idx < MaxSize) {
            DataY[Idx++] = y;
        }
        else {
            DataY.push_back(y);
            DataY.pop_front();
        }
    }
};

#endif