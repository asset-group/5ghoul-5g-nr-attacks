#!/usr/bin/env python2

from mpsse import *
from time import sleep

# Open mpsse in GPIO mode
io = MPSSE(GPIO)

# Toggle the first GPIO pin on/off 10 times
for i in range(0, 10):

    io.PinHigh(GPIOL0)
    print "GPIOL0 State:", io.PinState(GPIOL0)
    sleep(1)

    io.PinLow(GPIOL0)
    print "GPIOL0 State:", io.PinState(GPIOL0)
    sleep(1)

io.Close()
