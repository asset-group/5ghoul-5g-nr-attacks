#!/usr/bin/env python2

from mpsse import *
from time import sleep

# Open mpsse in bitbang mode
io = MPSSE(BITBANG);

# Set pin 0 high/low and read back pin 0's state 10 times
for i in range(0, 10):

    io.PinHigh(0)
    print "Pin 0 is:", io.PinState(0)
    sleep(1)

    io.PinLow(0)
    print "Pin 0 is:", io.PinState(0)
    sleep(1)

io.Close()
