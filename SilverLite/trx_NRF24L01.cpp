#include "trx.h"

#if defined(TRX_NRF)

#include <stdbool.h>
#include <stdint.h>
#include "binary.h"

extern "C" {
	#include "drv_xn297.h"
    extern void failloop( int );
}

//------------------------------------------------------------------------------
//#define RX_DATARATE_250K

#ifndef TX_POWER
#define TX_POWER 3
#endif

#define NRF_TO_TX B00000010
#define NRF_TO_RX B00000011

// crc enable - rx side
#define crc_en 1 // zero or one only
// CRC calculation takes 6 us at 168 MHz.

// xn297 to nrf24 emulation based on code from nrf24multipro by goebish
// DeviationTx by various contributors

static const uint8_t xn297_scramble[] = {
	0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
	0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
	0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc
};

// from https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
// reverse the bit order in a single byte
static uint8_t swapbits( uint8_t a )
{
	unsigned int b = a;
	b = ( ( b * 0x0802LU & 0x22110LU ) | ( b * 0x8020LU & 0x88440LU ) ) * 0x10101LU >> 16;
	return b;
}

static uint16_t crc16_update( uint16_t crc, uint8_t in )
{
	crc ^= in << 8;
	// manually unrolled loop for speed:
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	crc = ( crc & 0x8000 ) ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	return crc;
}

// crc calculated over address field (constant)
static uint16_t crc_addr = 0;

extern int crc_errors;

//------------------------------------------------------------------------------
// set both rx and tx address to a xn297 address
// the tx address can only send to another nrf24
// because it lacks the xn297 preamble
static void nrf24_set_xn297_address( uint8_t * addr )
{
	uint8_t rxaddr[ 6 ] = { 0x2a };    // 0x2a == (RX_ADDR_P0 | W_REGISTER)
	crc_addr = 0xb5d2;
	for ( int i = 5; i > 0; --i ) {
		rxaddr[ i ] = addr[ i - 1 ] ^ xn297_scramble[ 5 - i ];
		if ( crc_en ) {
			crc_addr = crc16_update( crc_addr, rxaddr[ i ] );
		}
	}

	// write rx address
	xn_writeregs( rxaddr, sizeof( rxaddr ) );
	// write tx address
	rxaddr[ 0 ] = 0x30; //  0x30 == (TX_ADDR | W_REGISTER)
	xn_writeregs( rxaddr, sizeof( rxaddr ) );
}


//------------------------------------------------------------------------------
static int nrf24_read_xn297_payload( int * rxdata, int size )
{
	xn_readpayload( rxdata, size );

	if ( crc_en ) {
		uint16_t crcx;
		crcx = crc_addr;
		for ( uint8_t i = 0; i < size - 2; ++i ) {
			crcx = crc16_update( crcx, rxdata[ i ] );
		}
		uint16_t crcrx = rxdata[ size - 2 ] << 8;
		crcrx |= rxdata[ size - 1 ] & 0xFF;

		// hardcoded for len 15
		if ( ( crcx ^ crcrx ) != 0x9BA7 ) {
			++crc_errors;
			return 0;
		}
	}
	for ( int i = 0; i < size - crc_en * 2; ++i ) {
		rxdata[ i ] = swapbits( rxdata[ i ] ^ xn297_scramble[ i + 5 ] );
	}

	// crc correct or not used
	return 1;
}

//------------------------------------------------------------------------------
static void nrf24_write_xn297_payload( int * txdata, int size )
{
	for ( int i = 0; i < size; ++i ) {
		txdata[ i ] = swapbits( txdata[ i ] ) ^ xn297_scramble[ i + 5 ];
	}

	xn_writepayload( txdata, size );
}

//------------------------------------------------------------------------------
void trx_Init()
{
	trx_spi_init();

	delay_us( 100 );

	// XXX, TODO: I'm fairly certain the following call to nrf24_set_xn297_address()
	// will not work as intended because SETUP_AW register has not been initialized
	// I deduced this because I wasn't getting any reception for bind packets
	// until I added an explicit call to: xn_writereg( SETUP_AW, 3 )
	// right before the nrf24_set_xn297_address(). Or by setting the address
	// *again* after the xn_writereg( SETUP_AW, 3 ) found below.
	// Note: I read and printed the value of SETUP_AW (here) before the call to
	// nrf24_set_xn297_address() below and found it had a value of 0. This is
	// weird because the docs say it should have a reset value of 0x3. Maybe
	// something to do with my hardware? Maybe I have a clone nrf24l01?
    static uint8_t rxaddr[ 5 ] = { 0, 0, 0, 0, 0 };
    nrf24_set_xn297_address( rxaddr );

    xn_writereg( EN_AA, 0 );      // aa disabled
    xn_writereg( EN_RXADDR, 1 );  // pipe 0 only
#ifdef RX_DATARATE_250K
    xn_writereg( RF_SETUP, B00100110 );     // power / data rate 250K
#else
    // xn_writereg( RF_SETUP, B00000110 );    // power / data rate 1000K
    xn_writereg( RF_SETUP, ( TX_POWER & 3 ) << 1 );    // power / data rate 1000K
#endif

    xn_writereg( RX_PW_P0, 15 + crc_en * 2 );  // payload size
    xn_writereg( SETUP_RETR, 0 ); // no retransmissions
    xn_writereg( SETUP_AW, 3 );   // address size (5 bytes)
    xn_writereg( RF_CH, 0 );      // bind on channel 0
    xn_command( FLUSH_RX );
    xn_writereg( 0, NRF_TO_RX );   // power up, crc disabled, rx mode

#define RADIO_CHECK
#ifdef RADIO_CHECK
    int rxcheck = xn_readreg( 0x0f ); // rx address pipe 5
    // should be 0xc6
    if ( rxcheck != 0xc6 ) {
        failloop( 3 );
    }
#endif

	// Set the address (because the earlier call to nrf24_set_xn297_address()
	// didn't take effect due to SETUP_AW not having been initialized)
	trx_SetAddr(nullptr);
}

//------------------------------------------------------------------------------
void trx_SetAddr(uint8_t addr[])
{
    uint8_t *rxaddress = addr;
    if (!rxaddress)
    {
        static uint8_t bindAddr[] = { 0, 0, 0, 0, 0 };
        rxaddress = bindAddr;
    }
    nrf24_set_xn297_address( rxaddress );
}

//------------------------------------------------------------------------------
void trx_SetChannel(uint8_t channel)
{
    // Note: 0x25 is (RF_CH | W_REGISTER)
    xn_writereg( 0x25, channel ); // Set channel frequency
}

//------------------------------------------------------------------------------
bool trx_TelemetrySent()
{
    return ( xn_readreg( 0x17 ) & B00010000 ) != 0;
}

//------------------------------------------------------------------------------
void trx_FlushTX()
{
    xn_command( FLUSH_TX );
}

//------------------------------------------------------------------------------
void trx_FlushRX()
{
    xn_command( FLUSH_RX );
}

//------------------------------------------------------------------------------
void trx_SetToRXMode()
{
    xn_writereg(0, NRF_TO_RX);
}

//------------------------------------------------------------------------------
void trx_PrepareToSendTelemetry()
{
    xn_writereg(0, 0);
    xn_writereg( 0, NRF_TO_TX );
}

//------------------------------------------------------------------------------
void trx_SendTelemetryPacket(int data[], int size)
{
    nrf24_write_xn297_payload(data, size);
}

//------------------------------------------------------------------------------
uint8_t trx_PacketAvailable()
{
	int status = xn_readreg( 7 );
#if 1
    if ( status & ( 1 << RX_DR ) ) { // RX packet received
        xn_writereg( STATUS, 1 << RX_DR ); // rx clear bit
        return 1;
    }
#else
    if ( ( status & B00001110 ) != B00001110 ) { // rx fifo not empty
        return 2;
    }
#endif
    return 0;
}

//------------------------------------------------------------------------------
bool trx_ReadPayload(int data[], int size)
{
    // Need to retrieve 2 additional bytes for the CRC value if crc check
    // is enabled. Note: data[] is guaranteed big enough to hold the additional bytes
    if (crc_en)
    {
        size += 2;
    }
    return nrf24_read_xn297_payload(data, size);
}

#endif
