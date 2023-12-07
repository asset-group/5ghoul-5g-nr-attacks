# ABOUT

[![Build Status](https://travis-ci.org/l29ah/libmpsse.svg?branch=master)](https://travis-ci.org/l29ah/libmpsse)

Libmpsse is a library for interfacing with SPI/I2C devices via FTDI's FT-2232 family of USB to serial
chips. Additionally, it provides control over the GPIO pins on the FTDI chips and supports a raw
bitbang mode as well. Based around the libftdi library, it is written in C and includes a Python wrapper 
courtesy of swig.

# Install

```bash
./build.sh # libmpsse-static.a and its examples and headers will be generated and copied to current folder
```

# Use Library

libmpsse requires linking to `pthreads` and `udev`.  You can compile an application by using the gcc example below:

```bash
gcc -o bitbang src/examples/bitbang.c -L. -I./src -lmpsse-static -lpthread -ludev
```

# LIBRARY API

In version 1.0 libmpsse was modified to support multiple simultaneous FTDI chips inside a single process,
which required a change to the API. Thus, the API for libmpsse versions prior to 1.0 are not compatible with 
the API for versions 1.0 and later.

The C and Python APIs are very similar, although the Python API has been made more "pythonic". Specifically,
in Python:

1. MPSSE is the name of the Python class, not of a method or function as it is in C. The class 
   constructor takes the same arguments as the MPSSE function in C. If you wish to instead use 
   the Open method to open a given device, simply do not specify these arguments when calling 
   the class constructor.

2. Due to its object-oriented nature, each class instance in Python internally handles the MPSSE
   context pointer; thus, none of the Python methods in the MPSSE class require this argument.

3. Size parameters required by the C functions are not used by the corresponding Python methods.

4. Besides the above described differences, all Python methods have a 1:1 compatibility with their
   respective C functions. However, in Python the class constructor (MPSSE) and Open method have 
   sensible defaults for several of their arguments so not all arguments are required. 

See the [README.C](docs/README.C) and [README.PYTHON.html](docs/README.PYTHON.html) files for more details.

# CODE EXAMPLES

There are SPI and I2C code examples for both C and Python in the `src/examples` directory. After installing
libmpsse, the C examples can be built by running:

    $ make example-code

There are more detailed descriptions of the SPI and I2C APIs in [README.SPI](docs/README.SPI) and [README.I2C](docs/README.I2C) files.

# BUILDING APPLICATIONS

To build applications in Python, you must import the `mpsse` module:

    from mpsse import *

To build applications in C, you must include the `mpsse.h` header file:

    #include <mpsse.h>

...and also link your program against the *libmpsse* library:

    $ gcc test.c -o test -lmpsse

# PHYSICAL CONNECTIONS

In order to speak to SPI or I2C devices, you must establish the proper physical connections between the target
device and your FTDI chip. The exact pin out of your target device and FTDI chip will differ based on the chips
in question, and the appropriate data sheets for each device should be referenced for this information.

Included in the docs directory is the FTDI MPSSE Basics application note (AN 135). Section 2 of this document
covers all of the pin configurations for each FTDI chip, as well as example diagrams regarding the physical
connections between the FTDI chip and your target device.

# KNOWN BUGS

The following are known bugs in libmpsse:

-   In SPI, the Transfer method fails if transfering large data chunks. Use the Read/Write functions 
    for transferring large chunks of data (~1MB or more).

# KNOWN LIMITATIONS

Libmpsse supports all four SPI modes as well as I2C. However, due to the design of the FTDI MPSSE implementation,
there are some limitations:

- All protocols support master mode only.
- SPI modes 1 and 3 are only partially supported; see the README.SPI for more information.
- In I2C, the SCL and SDA pins are open drain, but the MPSSE implementation explicitly drives these lines high.
  Although this is usually fine, it means that some I2C features, such as clock stretching, are not supported.

