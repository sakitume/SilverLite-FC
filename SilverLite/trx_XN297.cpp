#include "trx.h"

#if defined(TRX_XN297)

#include <stdbool.h>
#include <stdint.h>
#include "binary.h"

extern "C" {
#include "drv_xn297.h"
}

#ifndef TX_POWER
    #define TX_POWER 3
#endif

#define XN_TO_RX_XN297 B00001111
#define XN_TO_TX_XN297 B00000010
#define XN_TO_RX	XN_TO_RX_XN297
#define XN_TO_TX	XN_TO_TX_XN297

#define XN_POWER (B00000001 | ( ( TX_POWER & 3 ) << 1 ) )

//------------------------------------------------------------------------------
void trx_Init()
{
    trx_spi_init();

    static uint8_t bbcal[ 6 ] = { 0x3f, 0x4c, 0x84, 0x6F, 0x9c, 0x20 };
    xn_writeregs( bbcal, sizeof( bbcal ) );
    // new values
    static uint8_t rfcal[ 8 ] = { 0x3e, 0xc9, 0x9a, 0xA0, 0x61, 0xbb, 0xab, 0x9c };
    xn_writeregs( rfcal, sizeof( rfcal ) );
    // 0xa7 0x03
    static uint8_t demodcal[6] = { 0x39, 0x0b, 0xdf, 0xc4, B00100111, B00000000 };
    xn_writeregs( demodcal, sizeof( demodcal ) );

    delay_us( 100 );

    // write rx address " 0 0 0 0 0 "

    static uint8_t rxaddr[ 6 ] = { 0x2a, 0, 0, 0, 0, 0 };
    xn_writeregs( rxaddr, sizeof( rxaddr ) );

    xn_writereg( EN_AA, 0 );      // aa disabled
    xn_writereg( EN_RXADDR, 1 );  // pipe 0 only
    xn_writereg( RF_SETUP, XN_POWER );    // power / data rate / lna
    xn_writereg( RX_PW_P0, 15 );  // payload size
    xn_writereg( SETUP_RETR, 0 ); // no retransmissions (redundant?)
    xn_writereg( SETUP_AW, 3 );   // address size (5 bytes)
    xn_command( FLUSH_RX );
    xn_writereg( RF_CH, 0 );      // bind on channel 0

    xn_writereg( 0x1d, B00011000 );   // 64 bit payload, software ce

    xn_writereg( 0, XN_TO_RX );   // power up, crc enabled, rx mode

#ifdef RADIO_CHECK
    int rxcheck = xn_readreg( 0x0f ); // rx address pipe 5
    // should be 0xc6
    extern void failloop( int );
    if ( rxcheck != 0xc6 ) {
        failloop( 3 );
    }
#endif
}

//------------------------------------------------------------------------------
void trx_SetAddr(uint8_t addr[])
{
    const uint8_t *rxaddress = addr;
    if (!rxaddress)
    {
        static uint8_t bindAddr[] = { 0, 0, 0, 0, 0 };
        rxaddress = bindAddr;
    }

    uint8_t rxaddr_regs[ 6 ] = { 0x2a };    // 0x2a == (RX_ADDR_P0 | W_REGISTER)
    for ( int i = 1; i < 6; ++i ) {
        rxaddr_regs[ i ] = rxaddress[ i - 1 ];
    }
    // write new rx address
    xn_writeregs( rxaddr_regs, sizeof( rxaddr_regs ) );
    rxaddr_regs[ 0 ] = 0x30; // tx register (write) number, 0x30 == (TX_ADDR | W_REGISTER)

    // write new tx address
    xn_writeregs( rxaddr_regs, sizeof(rxaddr_regs) );
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
    xn_writereg(0, XN_TO_RX);
}

//------------------------------------------------------------------------------
void trx_PrepareToSendTelemetry()
{
    xn_command( FLUSH_TX );
    xn_writereg(0, XN_TO_TX);
}

//------------------------------------------------------------------------------
void trx_SendTelemetryPacket(int data[], int size)
{
    xn_writepayload( data, size );
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
    xn_readpayload( data, size );
    return true;
}

#endif