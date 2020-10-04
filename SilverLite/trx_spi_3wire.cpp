#if !defined(__MODULE_DEV__) && !defined(ARDUINO)

#include "trx.h"
#include "jee.h"

//------------------------------------------------------------------------------
#if defined(RX_SILVERLITE_BAYANG_PROTOCOL) && defined(TRX_SPI_3WIRE)

#if defined(TRX_SPI_3WIRE)
    #if !defined(TRX_XN297L)
        #warning "TRX_SPI_3WIRE defined, but TRX_XN297L isn't, are you sure about this?"
    #endif
#endif

// Instantiate the Pin<> objects needed for our software SPI implemenation
#include "trx_spi_config.h"

//------------------------------------------------------------------------------
#define DATA_HIGH 	SPI_DATA.write(1);
#define DATA_LOW 	SPI_DATA.write(0);
#define DATA_READ   SPI_DATA.read()

#define SCK_HIGH 	SPI_SCK.write(1);
#define SCK_LOW		SPI_SCK.write(0);

#define CS_ENABLE	TRX_CS_PIN.write(0);	// SPI chip select is active low
#define CS_DISABLE	TRX_CS_PIN.write(1);


//------------------------------------------------------------------------------
// SCK pulse will sometimes need to be lengthened by injecting a small delay
//
#define  __NOP()	__asm__ __volatile__("nop");
#if defined(__NOP)
//	#define DELAY_SLOW
	#define DELAY_SLOW __NOP(); __NOP();
//	#define DELAY_SLOW __NOP(); __NOP(); __NOP(); __NOP();
#else
    volatile static uint32_t count;
    #define DELAY_SLOW count = 1; while ( count-- );
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

static uint8_t data_out = 1;
//------------------------------------------------------------------------------
static void data_output( void )
{
	data_out = 1;
    SPI_DATA.mode(Pinmode::out);
}

//------------------------------------------------------------------------------
static void data_input( void )
{
	data_out = 0;
    SPI_DATA.mode(Pinmode::in_float);
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
    data_output();

	delay_us(1000);
}

//------------------------------------------------------------------------------
void trx_spi_write( int data )
{
	if ( ! data_out ) {
		data_output();
	}
	for ( int i =7; i >= 0; --i ) {
		SCK_LOW
		if ( ( data >> i ) & 1 ) {
			DATA_HIGH;
		} else {
			DATA_LOW;
		}
		static int count = 800;
		if ( count ) {  // Very ugly hack. It seems the XN297L needs its configuration bytes
			delay_us( 1 ); // be written very slow. After that, things can be sped up.
			--count;
		}
		SCK_HIGH;
		DELAY_SLOW
	}
	SCK_LOW
}

//------------------------------------------------------------------------------
int trx_spi_read()
{
	if ( data_out ) {
		data_input();
	}
	int recv = 0;
	for ( int i = 7; i >= 0; --i ) {
		recv = recv << 1;
		SCK_HIGH;
		DELAY_SLOW
		if ( DATA_READ ) {
			recv = recv | 1;
		}
		SCK_LOW
		DELAY_SLOW
	}
	return recv;
}

#endif  // #if defined(RX_SILVERLITE_BAYANG_PROTOCOL) && defined(TRX_SPI_3WIRE)
#endif 	// #if !defined(__MODULE_DEV__) && !defined(ARDUINO)

