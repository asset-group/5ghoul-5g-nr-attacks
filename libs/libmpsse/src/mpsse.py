import pylibmpsse as _mpsse

MPSSE_OK = _mpsse.MPSSE_OK
MPSSE_FAIL = _mpsse.MPSSE_FAIL

MSB = _mpsse.MSB
LSB = _mpsse.LSB

ACK = _mpsse.ACK
NACK = _mpsse.NACK

SPI0 = _mpsse.SPI0
SPI1 = _mpsse.SPI1
SPI2 = _mpsse.SPI2
SPI3 = _mpsse.SPI3
I2C = _mpsse.I2C
GPIO = _mpsse.GPIO
BITBANG = _mpsse.BITBANG

GPIOL0 = _mpsse.GPIOL0
GPIOL1 = _mpsse.GPIOL1
GPIOL2 = _mpsse.GPIOL2
GPIOL3 = _mpsse.GPIOL3
GPIOH0 = _mpsse.GPIOH0
GPIOH1 = _mpsse.GPIOH1
GPIOH2 = _mpsse.GPIOH2
GPIOH3 = _mpsse.GPIOH3
GPIOH4 = _mpsse.GPIOH4
GPIOH5 = _mpsse.GPIOH5
GPIOH6 = _mpsse.GPIOH6
GPIOH7 = _mpsse.GPIOH7

IFACE_ANY = _mpsse.IFACE_ANY
IFACE_A = _mpsse.IFACE_A
IFACE_B = _mpsse.IFACE_B
IFACE_C = _mpsse.IFACE_C
IFACE_D = _mpsse.IFACE_D

ONE_HUNDRED_KHZ = _mpsse.ONE_HUNDRED_KHZ
FOUR_HUNDRED_KHZ = _mpsse.FOUR_HUNDRED_KHZ
ONE_MHZ = _mpsse.ONE_MHZ
TWO_MHZ = _mpsse.TWO_MHZ
FIVE_MHZ = _mpsse.FIVE_MHZ
SIX_MHZ = _mpsse.SIX_MHZ
TEN_MHZ = _mpsse.TEN_MHZ
TWELVE_MHZ = _mpsse.TWELVE_MHZ
FIFTEEN_MHZ = _mpsse.FIFTEEN_MHZ
THIRTY_MHZ = _mpsse.THIRTY_MHZ

class MPSSE(object):
    """
    Python class wrapper for libmpsse.
    """

    def __init__(self, mode=None, frequency=ONE_HUNDRED_KHZ, endianess=MSB):
        """
        Class constructor.

        @mode      - The MPSSE mode to use, one of: SPI0, SPI1, SPI2, SPI3, I2C, GPIO, BITBANG, None.
                             If None, no attempt will be made to connect to the FTDI chip. Use this if you want to call the Open method manually.
        @frequency - The frequency to use for the specified serial protocol, in hertz (default: 100KHz).
        @endianess - The endianess of data transfers, one of: MSB, LSB (default: MSB).

        Returns None.
        """
        self.context = None
        if mode is not None:
            self.context = _mpsse.MPSSE(mode, frequency, endianess)
            if self.context.open == 0:
                raise Exception(self.ErrorString())

    def __enter__(self):
        return self

    def __exit__(self, t, v, traceback):
        if self.context:
            self.Close()

    def __del__(self):
        if self.context:
            self.Close()

    def Open(self, vid, pid, mode, frequency=ONE_HUNDRED_KHZ, endianess=MSB, interface=IFACE_A, description=None, serial=None, index=0):
        """
        Opens the specified FTDI device.
        Called internally by __init__ if the __init__ mode is not None.

        @vid         - FTDI USB vendor ID.
        @pid         - FTDI USB product ID.
        @mode        - The MPSSE mode to use, one of: SPI0, SPI1, SPI2, SPI3, I2C, GPIO, BITBANG.
        @frequency   - The frequency to use for the specified serial protocol, in hertz (default: 100KHz).
        @endianess   - The endianess of data transfers, one of: MSB, LSB (default: MSB).
        @interface   - The interface to use on the FTDI chip, one of: IFACE_A, IFACE_B, IFACE_C, IFACE_D, IFACE_ANY (default: IFACE_A).
        @description - FTDI device product description (default: None).
        @serial      - FTDI device serial number (default: None).
        @index       - Number of matching device to open if there are more than one, starts with zero (default: 0).

        Returns MPSSE_OK on success.
        Raises an exeption on failure.
        """
        self.context = _mpsse.OpenIndex(vid, pid, mode, frequency, endianess, interface, description, serial, index)
        if self.context.open == 0:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def Close(self):
        """
        Closes the FTDI device connection, deinitializes libftdi, and frees the libmpsse context.

        Returns None.
        """
        retval = _mpsse.Close(self.context)
        self.context = None

    def ErrorString(self):
        """
        Returns the last libftdi error string.
        """
        return _mpsse.ErrorString(self.context)

    def SetMode(self, mode, endianess):
        """
        Sets the appropriate transmit and receive commands based on the requested mode and byte order.
        Called internally by __init__ and Open.

        @mode      - The MPSSE mode to use, one of: SPI0, SPI1, SPI2, SPI3, I2C, GPIO, BITBANG.
        @endianess - The endianess of data transfers, one of: MSB, LSB.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.SetMode(self.context, mode, endianess) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def EnableBitmode(self, tf):
        """
        Enables/disables bitwise data transfers.
        Called internally by ReadBits and WriteBites.

        @tf - Set to 1 to enable bitwise transfers, 0 to disable.

        Returns None.
        """
        _mpsse.EnableBitmode(self.context, tf)

    def FlushAfterRead(self, tf):
        """
        Enables / disables the explicit flushing of the recieve buffer after each read operation.

        @tf - Set to 1 to enable flushing, 0 to disable (disabled by default).

        Returns None.
        """
        return _mpsse.FlushAfterRead(self.context, tf)

    def SetClock(self, frequency):
        """
        Sets the appropriate divisor for the desired clock frequency.
        Called internally by __init__ and Open.

        @frequency - The desired clock frequency, in hertz.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.SetClock(self.context, frequency) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def GetClock(self):
        """
        Returns the currently configured clock rate, in hertz.
        """
        return _mpsse.GetClock(self.context)

    def GetVid(self):
        """
        Returns the vendor ID of the FTDI chip.
        """
        return _mpsse.GetVid(self.context)

    def GetPid(self):
        """
        Returns the product ID of the FTDI chip.
        """
        return _mpsse.GetPid(self.context)

    def GetDescription(self):
        """
        Returns the description of the FTDI chip, if any.
        This will only be populated if __init__ was used to open the device.
        """
        return _mpsse.GetDescription(self.context)

    def SetLoopback(self, enable):
        """
        Enable / disable internal loopback. Loopback is disabled by default.

        @enable - Set to 1 to enable loopback, 0 to disable (disabled by default).

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.SetLoopback(self.context, enable) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def SetCSIdle(self, idle):
        """
        Sets the idle state of the chip select pin.

        @idle - Set to 1 to idle high, 0 to idle low (CS idles high by default).

        Returns None.
        """
        _mpsse.SetCSIdle(self.context, idle)

    def Start(self):
        """
        Send data start condition.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.Start(self.context) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def Stop(self):
        """
        Send data stop condition.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.Stop(self.context) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def Write(self, data):
        """
        Writes bytes out via the selected serial protocol.

        @data - A string of bytes to be written.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.Write(self.context, data, len(data)) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def Read(self, size):
        """
        Reads bytes over the selected serial protocol.

        @size - Number of bytes to read.

        Returns a string of size bytes.
        """
        return _mpsse.Read(self.context, size)

    def Transfer(self, data):
        """
        Transfers data over the selected serial protocol.
        For use only in SPI0, SPI1, SPI2, SPI3 modes.

        @data - A string of bytes to be written.

        Returns a string of len(data) bytes.
        """
        return _mpsse.Transfer(self.context, data)

    def SetAck(self, ack):
        """
        Sets the transmitted ACK bit.
        For use only in I2C mode.

        @ack - One of: ACK, NACK.

        Returns None.
        """
        _mpsse.SetAck(self.context, ack)

    def SendAcks(self):
        """
        Causes all subsequent I2C read operations to respond with an acknowledgement.

        Returns None.
        """
        _mpsse.SendAcks(self.context)

    def SendNacks(self):
        """
        Causes all subsequent I2C read operations to respond with a no-acknowledgement.

        Returns None.
        """
        return _mpsse.SendNacks(self.context)

    def GetAck(self):
        """
        Returns the last received ACK bit.

        Returns one of: ACK, NACK.
        """
        return _mpsse.GetAck(self.context)

    def PinHigh(self, pin):
        """
        Sets the specified GPIO pin high.

        @pin - Pin number 0 - 11 in GPIO mode.
               In all other modes, one of: GPIOL0, GPIOL1, GPIOL2, GPIOL3, GPIOH0, GPIOH1, GPIOH2, GPIOH3, GPIOH4, GPIOH5, GPIOH6, GPIOH7.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.PinHigh(self.context, pin) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def PinLow(self, pin):
        """
        Sets the specified GPIO pin low.

        @pin - Pin number 0 - 11 in GPIO mode.
               In all other modes, one of: GPIOL0, GPIOL1, GPIOL2, GPIOL3, GPIOH0, GPIOH1, GPIOH2, GPIOH3, GPIOH4, GPIOH5, GPIOH6, GPIOH7.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.PinLow(self.context, pin) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def SetDirection(self, direction):
        """
        Sets the input/output direction of pins as determined by direction (1 = Output, 0 = Input).
        For use in BITBANG mode only.

        @direction -  Byte indicating input/output direction of each bit (1 is output, 0 is input).

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.SetDirection(self.context, direction) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def WriteBits(self, bits, n):
        """
        Performs a bitwise write of up to 8 bits at a time.

        @bits - An integer of bits to be written.
        @n    - Transmit n number of least-significant bits.

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.WriteBits(self.context, bits, n) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def ReadBits(self, n):
        """
        Performs a bitwise read of up to 8 bits at a time.

        @n - Number of bits to read.

        Returns an integer value with the read bits set.
        """
        return ord(_mpsse.ReadBits(self.context, n))

    def WritePins(self, data):
        """
        Writes a new state to the chip's pins.
        For use in BITBANG mode only.

        @data - An integer with the bits set to the desired pin states (1 = output, 0 = input).

        Returns MPSSE_OK on success.
        Raises an exception on failure.
        """
        if _mpsse.WritePins(self.context, data) == MPSSE_FAIL:
            raise Exception(self.ErrorString())
        return MPSSE_OK

    def ReadPins(self):
        """
        Reads the current state of the chip's pins.
        For use in BITBANG mode only.

        Returns an integer with the corresponding pin's bits set.
        """
        return _mpsse.ReadPins(self.context)

    def PinState(self, pin, state=-1):
        """
        Checks the current state of the pins.
        For use in BITBANG mode only.

        @pin   - The pin number whose state you want to check.
        @state - The value returned by ReadPins. If not specified, ReadPins will be called automatically.

        Returns a 1 if the pin is high, 0 if the pin is low.
        """
        return _mpsse.PinState(self.context, pin, state)

    def Tristate(self):
        """
        Puts all I/O pins into a tristate mode (FT232H only).
        """
        return _mpsse.Tristate(self.context)

    def Version(self):
        """
        Returns the libmpsse version number.
        High nibble is major, low nibble is minor.
        """
        return _mpsse.Version()
