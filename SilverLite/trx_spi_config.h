#if defined(__TRX_SPI_CONFIG_H__)
    #error "trx_spi_config.h must be included in only a single .cpp file"
#else
#define __TRX_SPI_CONFIG_H__

//------------------------------------------------------------------------------
// This file is where you specify the pins you're using to connect your 
// transceiver module to your flight controller board.
//
// Supported transceiver modules are:
//  * NRF24L01
//  * LT8900
//  * XN297
//  * XN297L
//
// Note: _my_config.h is where you define what transceiver module you're using
// and if you're using 3-wire SPI instead of 4-wire SPI
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// OMINBUSF4 exposes many pads which we could use for software SPI connection
// to a transceiver module.
//
//  [TX1]           PA9     Used for SPI_MOSI 
//  [RX1(DSM)]      PA10    Used for SPI_MISO (or SPI_DATA if 3-wire)
//  [LED_STRIP]     PD2     Used for SPI_CLK 
//  [SBUS/PPM]      PB14    Used for TRX_CS_PIN 
//

//------------------------------------------------------------------------------
// The available pads on the NOX Play F4 board are:
//	DSM/IBUS/PPM	- PB10 (verified with multimeter) has pullup (reads 3.3v)
//	TX1				- PB6 (verified by code) has pullup (reads 3.3v)
//	RX1      		- PB7 (verified with multimeter) has pullup (reads 3.3v)
//	TX2       		- PA2 (verified with multimeter) has pullup (reads 3.3v)
//	LED_STRIP 		- PA0 (verified with multimeter) floating or held almost low (reads 0.09v)
//
// Note: Original SilF4Ware for NOX target uses USART2 (TX2) for blackbox logging. 
// This means the startup code is configuring some peripherals (DMA, USART2, GPIO).
// I can't recall if I had to reconfigure any of that by editing the the file
// `Targets/NOX/STM32F411 NOXE.ioc` with STM32CubeMX. It looks like I left it
// be and perhaps the code in `trx_spi_init()` configures PA2(TX2) to be
// OUTPUT mode rather than alternate function of UART2 TX.

#if defined(TRX_SPI_3WIRE) && defined(OMNIBUSF4)
//------------------------------------------------------------------------------
// 3-wire SPI pin configuration for OMNIBUSF4 target
static Pin<'B', 14>		TRX_CS_PIN;	    // PB14
static Pin<'D', 2>		SPI_SCK;	    // PD2
static Pin<'A', 9>      SPI_DATA;       // PA9
#endif

#if !defined(TRX_SPI_3WIRE) && defined(OMNIBUSF4)
//------------------------------------------------------------------------------
// 4-wire SPI pin configuration for OMNIBUSF4 target
static Pin<'B', 14>		TRX_CS_PIN;	    // PB14
static Pin<'D', 2>		SPI_SCK;	    // PD2
static Pin<'A', 10>		SPI_MISO;	    // PA10
static Pin<'A', 9>		SPI_MOSI;	    // PA9
#endif

#if defined(TRX_SPI_3WIRE) && defined(NOX)
//------------------------------------------------------------------------------
// 3-wire SPI pin configuration for NOX target
static Pin<'A', 0>		TRX_CS_PIN;	    // PA0		LED_STRIP
static Pin<'A', 2>		SPI_SCK;	    // PA2		TX2
static Pin<'B', 10> 	SPI_DATA;	    // PB10		DSM/IBUS/PPM
#endif

#if !defined(TRX_SPI_3WIRE) && defined(NOX)
//------------------------------------------------------------------------------
// 4-wire SPI pin configuration for NOX target
static Pin<'A', 2>		TRX_CS_PIN;	    // PA2		TX2
static Pin<'A', 0>		SPI_SCK;	    // PA0		LED_STRIP
static Pin<'B', 6>	    SPI_MOSI;	    // PB6		TX1
static Pin<'B', 10>	    SPI_MISO;	    // PB10		DSM/IBUS/PPM
#endif


#if defined(TRX_LT8900)
//------------------------------------------------------------------------------
// LT8900 implementation requires we strobe the LT8900 RESET pin.
static Pin<'B', 13>     TRX_RESET_PIN;  // PB13
#endif

#if defined(NOX)
//------------------------------------------------------------------------------
// NOX Play F4 board has a pad labeled SBUS which I believe goes through
// a configurable inverter before connecting to PA3. Below are some pin definitions
// you can try using should you wish to experiment with that pad
//static Pin<'A', 3>		SBUS_INPUT;		// SBUS pad on fc board via inverter
//static Pin<'C', 14>		SBUS_INVERTER;	// Inverter enable/disable for SBUS input
#endif

#endif  // __TRX_SPI_CONFIG_H__
