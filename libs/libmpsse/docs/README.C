MPSSE FUNCTIONS


	struct mpsse_context *MPSSE.Open(int vid, int pid, enum modes mode, int freq, int endianess, int interface, const char *description, const char *serial)

		Opens the device matching the given vendor and product ID, description, or serial number. 
		This can be used to create an MPSSE context for any supported mode.

		@vid         - Device vendor ID.
		@pid         - Device product ID.
		@mode        - MPSSE mode, one of: SPI0, SPI1, SPI2, SPI3, I2C, MCU8, MCU16, GPIO, BITBANG.
		@freq        - Clock frequency to use for the specified mode.
		@endianess   - Specifies how data is clocked in/out (MSB, LSB).
		@interface   - FTDI interface to use, one of: IFACE_ANY, IFACE_A - IFACE_D.
		@description - Device product description (set to NULL if not needed).
		@serial	     - Serial number of the FTDI (set to NULL if not needed).

		Returns a pointer to an MPSSE context structure. 
		On success, mpsse->open will be set to 1.
		On failure, mpsse->open will be set to 0.


	void MPSSE.Close(struct mpsse_context *mpsse)
	 
		Closes the FTDI device and deinitializes libftdi.

		@mpsse - MPSSE context pointer.

		Returns void.


	char *MPSSE.ErrorString(struct mpsse_context *mpsse)

		Get the last error string as reported by libftdi. This data should not be freed by the caller.

		@mpsse - MPSSE context pointer.

		Returns a pointer to the last error string reported by libftdi.


	int MPSSE.GetVid(struct mpsse_context *mpsse)

		Get the USB vendor ID of the connected FTDI device.

		@mpsse - MPSSE context pointer.

		Returns the FTDI vendor ID.


	int MPSSE.GetPid(struct mpsse_context *mpsse)

		Get the USB product ID of the connected FTDI device.

		@mpsse - MPSSE context pointer.

		Returns the FTDI product ID.


	char *MPSSE.GetDescription(struct mpsse_context *mpsse)

		Get the description, as listed in the internal supported_devices structure, of the connected FTDI device.
		The caller must not free the return value.

		@mpsse - MPSSE context pointer.

		Returns a pointer to the device description string.

	int MPSSE.GetClock(struct mpsse_context *mpsse)

		Gets the currently configured clock frequency.

		@mpsse - MPSSE context pointer.

		Returns an unsigned 32 bit value representing the clock frequency.


	int MPSSE.SetClock(struct mpsse_context *mpsse, int freq)

		Sets the appropriate divisor for the desired clock frequency.
		Called internally by Open().

		@mpsse - MPSSE context pointer.
		@freq  - Desired clock frequency in hertz.

		Returns MPSSE_OK on success.
		Returns MPSSE_FAIL on failure.


	int MPSSE.SetMode(struct mpsse_context *mpsse, enum modes mode, int endianess)

		Sets the appropriate transmit and receive commands based on the requested mode and byte order.
		Called internally by Open().

		@mpsse     - MPSSE context pointer.
		@mode      - The desired mode, as listed in enum modes.
		@endianess - Most or least significant byte first (MSB / LSB).

		Returns MPSSE_OK on success.
		Returns MPSSE_FAIL on failure.


	int MPSSE.SetLoopback(struct mpsse_context *mpsse, int enable)

		Enable / disable internal loopback.
		Called internally by SetMode().

		@mpsse  - MPSSE context pointer.
		@enable - Zero to disable the FTDI internal loopback, 1 to enable loopback.

		Returns MPSSE_OK on success.
		Returns MPSSE_FAIL on failure.


SPI FUNCTIONS


	int MPSSE.SPI.Start(struct mpsse_context *mpsse)

                Asserts the chip select pin.

		@mpsse - MPSSE context pointer.
                
		Returns MPSSE_OK on success.
                Returns MPSSE_FAIL on failure.


	int MPSSE.SPI.Stop(struct mpsse_context *mpsse)

                Deasserts the chip select pin.

		@mpsse - MPSSE context pointer.

                Returns MPSSE_OK on success.
                Returns MPSSE_FAIL on failure.


	char *MPSSE.SPI.Read(struct mpsse_context *mpsse, int size)

                Reads SPI data.

		@mpsse - MPSSE context pointer.
                @size  - Number of bytes to read.

                Returns a pointer to the read data on success.
                Returns NULL on failure.


        int MPSSE.SPI.Write(struct mpsse_context *mpsse, char *data, int size)

                Writes SPI data.

		@mpsse - MPSSE context pointer.
                @data  - Buffer of data to send.
                @size  - Size of data.

                Returns MPSSE_OK on success.
                Returns MPSSE_FAIL on failure.


	char *MPSSE.SPI.Transfer(struct mpsse_context *mpsse, char *outbuf, int size)

		Performs a bi-directional data transfer of size bytes.

		@mpsse  - MPSSE context pointer.
		@outbuf - Buffer of bytes to send.
		@size   - Size of outbuf.

		Returns a pointer to the read data on success.
		Returns NULL on failure.


I2C FUNCTIONS


	int MPSSE.I2C.GetAck(struct mpsse_context *mpsse)

		Return the status of the last received acknolwedgement bit.

		@mpsse - MPSSE context pointer.

		Returns 0 if an ACK was received.
		Returns 1 if a NACK was received.


	void MPSSE.I2C.SetAck(struct mpsse_context *mpsse, int ack)

		Set the ACK bit to send when Read() receives a byte from the I2C slave device.

		@mpsse - MPSSE context pointer.
		@ack   - Set to 0 to send ACKs (default).
		         Set to 1 to send NACKs.

		Returns void.


	void MPSSE.I2C.SendAcks(struct mpsse_context *mpsse)
	
		Causes libmpsse to send ACKs after each read byte in I2C mode.

		@mpsse - MPSSE context pointer.
		
		Returns void.


	void MPSSE.I2C.SendNacks(struct mpsse_context *mpsse)

		Causes libmpsse to send NACKs after each read byte in I2C mode.

		@mpsse - MPSSE context pointer.

		Returns void.


	

	void SetCSIdle(struct mpsse_context *mpsse, int idle)

		Set the idle state of the chip select pin. CS idles high by default.

		@mpsse - MPSSE context pointer.
		@idle  - Set to 1 to idle high, 0 to idle low.

		Returns void.


	int PinHigh(struct mpsse_context *mpsse, int pin)

		Set the specified GPIO pin high (1).
		Note that the state of the GPOL pins can only be changed prior to a Start()
		or after a Stop() condition is sent. 

		@mpsse - MPSSE context pointer.
		@pin   - Pin number, one of: 0 - 11.

		Returns MPSSE_OK on success.
		Returns MPSSE_FAIL on failure.


	int PinLow(struct mpsse_context *mpsse, int pin)

		Set the specified GPIO pin low (0).
		Note that the state of GPIO pins 0-3 can only be changed prior to a Start()
		or after a Stop() condition is sent. 

		@mpsse - MPSSE context pointer.
		@pin   - Pin number, one of: 0 - 11.

		Returns MPSSE_OK on success.
		Returns MPSSE_FAIL on failure.


	int ReadPins(struct mpsse_context *mpsse);

		Reads the state of the chip's pins.

		@mpsse - MPSSE context pointer.

		Returns a byte with the corresponding pin's bits set to 1 or 0.


	int PinState(struct mpsse_context *mpsse, int pin, int state)

		Checks if a specific pin is high or low.

		@mpsse - MPSSE context pointer.
		@pin   - The pin number.
		@state - The state of the pins, as returned by ReadPins.
		         If set to -1, ReadPins will automatically be called.
		
		Returns a 1 if the pin is high, 0 if the pin is low.




DEFINITIONS


	Interface definitions, used for the 'interface' argument to Open():

	        IFACE_ANY       = INTERFACE_ANY,
	        IFACE_A         = INTERFACE_A,
	        IFACE_B         = INTERFACE_B,
	        IFACE_C         = INTERFACE_C,
	        IFACE_D         = INTERFACE_D


	Clock rate definitions, provided for convenience, used for the 'frequency' argument of MPSSE() and Open():

	        ONE_HUNDRED_KHZ  = 100000,
	        FOUR_HUNDRED_KHZ = 400000,
	        ONE_MHZ          = 1000000,
	        TWO_MHZ          = 2000000,
	        FIVE_MHZ         = 5000000,
	        SIX_MHZ          = 6000000,
	        TEN_MHZ          = 10000000,
	        TWELVE_MHZ       = 12000000,
	        THIRTY_MHZ       = 30000000,


	Mode definitions, used for the 'mode' argument of MPSSE() and Open():

	        SPI0    = 1,
	        SPI1    = 2,
	        SPI2    = 3,
	        SPI3    = 4,
	        I2C     = 5,
	        GPIO    = 6
		BITBANG = 7

	GPIO pin definitions:

		GPIOL0 = 0,
		GPIOL1 = 1,
		GPIOL2 = 2,
		GPIOL3 = 3,
		GPIOH0 = 4,
		GPIOH1 = 5,
		GPIOH2 = 6,
		GPIOH3 = 7,
		GPIOH4 = 8,
		GPIOH5 = 9,
		GPIOH6 = 10,
		GPIOH7 = 11

	MPSSE return codes; most functions that return an integer value return one of these unless otherwise specified:

		MPSSE_OK   =  0,
		MPSSE_FAIL = -1


