//------------------------------------------------------------------------------
// This is where you define the SPI pins used by your target hardware for
// communicating with the onboard MAX7456 chip.
//------------------------------------------------------------------------------

#if defined(OMNIBUS)
//------------------------------------------------------------------------------
// OMINBUSF4 connects to the MAX7456 via SPI using the following pins
//
static Pin<'A', 15>		OSD_CS_Pin;	// A15
static Pin<'C', 10>		SPI_SCK;	// C10
static Pin<'C', 11>		SPI_MISO;	// C11
static Pin<'C', 12>		SPI_MOSI;	// C12

#elif defined(NOX)
//------------------------------------------------------------------------------
// NOX connects to the MAX7456 via SPI using the following pins
//
static Pin<'A', 10>		OSD_CS_Pin;	// A10
static Pin<'B', 13>		SPI_SCK;	// B13
static Pin<'B', 14>		SPI_MISO;	// B14
static Pin<'B', 15>		SPI_MOSI;	// B15
#endif

