#if !defined(__MODULE_DEV__) && !defined(ARDUINO)

#include "trx.h"
#include "jee.h"

//------------------------------------------------------------------------------
// Instantiate the Pin<> objects needed for our software SPI implemenation
#include "trx_spi_config.h"

//------------------------------------------------------------------------------
#define MOSIHIGH 	SPI_MOSI.write(1);
#define MOSILOW 	SPI_MOSI.write(0);
#define SCKHIGH 	SPI_SCK.write(1);
#define SCKLOW 		SPI_SCK.write(0);

#define READMISO 	SPI_MISO.read()
#define CS_ENABLE	TRX_CS_PIN.write(0);	// SPI chip select is active low
#define CS_DISABLE	TRX_CS_PIN.write(1);


//------------------------------------------------------------------------------
// SCK pulse will sometimes need to be lengthened by injecting a small delay
//
#define  __NOP()	__asm__ __volatile__("nop");
#if defined(__NOP)
//	#define DELAY_SLOW __NOP(); __NOP(); __NOP(); __NOP();
	#define DELAY_SLOW
#else
#define DELAY_SLOW count = 1; while ( count-- );
#endif

volatile static uint32_t count;

#define _NOP_	__asm__ __volatile__("nop");

#if defined(TRX_LT8900)
	#define DELAY_O3 count = 0; while ( count-- );
#else	
	#define	DELAY_O3	
	//#define DELAY_O3 _NOP_ _NOP_ _NOP_ _NOP_ _NOP_ _NOP_ _NOP_ _NOP_ _NOP_// necessary when the loop is unrolled i.e. with -O3
	//#define DELAY_O3 count = 2; while ( count-- );
#endif	

//------------------------------------------------------------------------------
void trx_spi_cs_enable()
{
    CS_ENABLE
}

//------------------------------------------------------------------------------
void trx_spi_cs_disable()
{
    CS_DISABLE
}

//------------------------------------------------------------------------------
void trx_spi_init()
{
    TRX_CS_PIN.mode(Pinmode::out);
    TRX_CS_PIN.write(1);

#if defined(TRX_LT8900)
    // Need to strobe the reset pin (see p.25 of datasheet)
    #if 1   // Following works well
    TRX_RESET_PIN.mode(Pinmode::out);
    TRX_RESET_PIN.write(0); // strobe reset pin (active low)
    delay_ms(25);
    TRX_RESET_PIN.write(1); // strobe reset pin (active low)
    delay_ms(25);   // per spec sheet we should wait 1 to 5 ms for crystal oscillator to stabilize before initializing LT8900 registers
    #else   // ...but the following is what I had to do with Arduino version (eliminate last delay)
    TRX_RESET_PIN.mode(Pinmode::out);
    TRX_RESET_PIN.write(0); // strobe reset pin (active low)
    delay_ms(25);
    TRX_RESET_PIN.write(1);
    #endif
#endif

	SPI_SCK.mode(Pinmode::out);
	SPI_MISO.mode(Pinmode::in_float);
	SPI_MOSI.mode(Pinmode::out);

	delay_us(1000);
}

//------------------------------------------------------------------------------
void trx_spi_write( int data )
{
//#define __USE__ORIGINAL__
#if defined(__USE__ORIGINAL__)	
	for ( int i =7; i >= 0; --i ) {
		SCKLOW
		if ( ( data >> i ) & 1 ) {
			MOSIHIGH
		} else {
			MOSILOW
		}
		DELAY_SLOW
		SCKHIGH
		DELAY_SLOW
	}
	SCKLOW
	MOSILOW
	DELAY_SLOW
#else
	for ( int i =7; i >= 0; --i ) {
		SCKLOW
		if ( ( data >> i ) & 1 ) {
			MOSIHIGH
		} else {
			MOSILOW
		}
		SCKHIGH
		DELAY_O3
	}
	SCKLOW
	MOSILOW
#endif	
}

//------------------------------------------------------------------------------
int trx_spi_read()
{
#if defined(__USE__ORIGINAL__)	
	int recv = 0;
	MOSILOW
	for ( int i = 7; i >= 0; --i ) {
		recv = recv << 1;
		SCKHIGH
		DELAY_SLOW
		if ( READMISO ) {
			recv = recv | 1;
		}
		SCKLOW
		DELAY_SLOW
	}
	return recv;
#else
	int recv = 0;
	MOSILOW
	for ( int i = 7; i >= 0; --i ) {
		recv = recv << 1;
		SCKHIGH
		DELAY_O3
		if ( READMISO ) {
			recv = recv | 1;
		}
		SCKLOW
		DELAY_O3
	}
	return recv;
#endif	
}

#endif
