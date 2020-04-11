//------------------------------------------------------------------------------
// This is where you specify the pins you're using to connect the NRF24L01
// (or LT8900) transceiver to your flight controller board.
//------------------------------------------------------------------------------


#if defined(TRX_LT8900)
//------------------------------------------------------------------------------
// OMINBUSF4 exposes these pads which we'll use for software SPI connection
// to LT8900:
//
//  SPI_MOSI [TX1]          PA9
//  SPI_MISO [RX1(DSM)]     PA10
//  SPI_CLK [LED_STRIP]     PD2
//  SPI_XN_SS [SBUS/PPM]    PB14
//
//  Note: I can confirm that using SBUS/PPM on PB14 works fine for CS
//  I had previously doubted it due to my initial attempts failing
//  (but I think that was due to not strobing the reset pin which
//  I now do).
//
static Pin<'B', 13>     TRX_RESET_PIN;  // PB13
#endif

#if defined(OMNIBUSF4)
//------------------------------------------------------------------------------
//
static Pin<'B', 14>		TRX_CS_PIN;	    // PB14
static Pin<'D', 2>		SPI_SCK;	    // PD2
static Pin<'A', 10>		SPI_MISO;	    // PA10
static Pin<'A', 9>		SPI_MOSI;	    // PA9

#elif defined(NOX)
//------------------------------------------------------------------------------
//	DSM/IBUS/PPM	- PB10 (verified with multimeter) has pullup (reads 3.3v)
//	TX1				- PB6 (verified by code) has pullup (reads 3.3v)
//	RX1      		- PB7 (verified with multimeter) has pullup (reads 3.3v)
//	TX2       		- PA2 (verified with multimeter) has pullup (reads 3.3v)
//	LED_STRIP 		- PA0 (verified with multimeter) floating or held almost low (reads 0.09v)
//
// Note: SilF4Ware uses USART2 (TX2) for blackbox logging. This means the startup
// code is configuring some peripherals (DMA, USART2, GPIO) that we'll need
// to either work around, or disable (via STM32CubeMX), or we simply avoid
// using TX2. 

//#define OLD_NOX_PINOUT
#if defined(OLD_NOX_PINOUT)
static Pin<'B', 7>		TRX_CS_PIN;	    // PB7 		RX1
static Pin<'A', 0>		SPI_SCK;	    // PA0		LED_STRIP
static Pin<'B', 6>		SPI_MOSI;	    // PB6		TX1
static Pin<'B', 10>		SPI_MISO;	    // PB10		DSM/IBUS/PPM
#else
static Pin<'A', 2>		TRX_CS_PIN;	    // PA2		TX2
static Pin<'A', 0>		SPI_SCK;	    // PA0		LED_STRIP
static Pin<'B', 6>		SPI_MOSI;	    // PB6		TX1
static Pin<'B', 10>		SPI_MISO;	    // PB10		DSM/IBUS/PPM
#endif

// I may want to experiment with these two pins should I need more IO
//static Pin<'A', 3>		SBUS_INPUT;		// SBUS pad on fc board via inverter
//static Pin<'C', 14>		SBUS_INVERTER;	// Inverter enable/disable for SBUS input
#endif
