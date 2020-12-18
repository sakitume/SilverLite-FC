#include "config.h"
#include "console.h"

//------------------------------------------------------------------------------
#if !defined(STM32F303xC)
// See: https://jeelabs.org/2018/getting-started-f407/
#include "jee.h"
#include "arch/stm32f4-usb.h"
UsbDev console;
#else
#include "jee.h"
#include "f3_console.h"
static f3Console console;
#endif

//------------------------------------------------------------------------------
extern "C" {
    extern const char *tprintf(const char* fmt, ...);
    extern void jump_to_bootloader( void );

#if defined(RX_FLYSKY)
    extern void rxSpiBind(void);
    extern void A7105Shutdown();
#endif    
}

//------------------------------------------------------------------------------
void console_poll(void)
{
    // Echo console input.
    // 
    while (console.readable())
    {
        int ch = console.getc();
        if (ch == 'r')
        {
#if defined(RX_FLYSKY)
            // Disable interrupts used by A7105 code
            A7105Shutdown();
#endif
            jump_to_bootloader();
        }
        else if (ch == 'b')
        {
#if defined(RX_FLYSKY)
            rxSpiBind();
#endif    
        }
        console.putc(ch);
    }
}

//------------------------------------------------------------------------------
void console_init(void)
{
    console.init();
}

//------------------------------------------------------------------------------
uint8_t console_packet_buffer[CONSOLE_PACKET_BUFFER_MAX];
static uint8_t packetIndex;

//------------------------------------------------------------------------------
/*
    See: https://stackoverflow.com/questions/815758/simple-serial-point-to-point-communication-protocol

    // total packet length minus flags len+4
    U8 sflag;   //0x7e start of packet end of packet flag from HDLC
    U8 cmd;     //tells the other side what to do.
    U8 len;     // payload length
    U8 payload[len];  // could be zero len
    U16 crc;    // in little endian
    U8 eflag;   //end of frame flag

*/
void console_sendPacket(uint8_t cmdID, uint8_t *payload, uint8_t payloadLen)
{
    console.putc(0x7E); // start of frame flag
    console.putc(cmdID);
    console.putc(payloadLen);
    int sum = 0x7E + cmdID + payloadLen;
    for (unsigned i=0; i<payloadLen; i++)
    {
        uint8_t c = payload[i];
        sum += c;
        console.putc(c);
    }
    console.putc(sum & 0xFF);
    console.putc((sum >> 8) & 0xFF);
    console.putc(0x7E); // end of frame flag
}

//------------------------------------------------------------------------------
void console_openPacket(void)
{
    packetIndex = 0;
}

//------------------------------------------------------------------------------
void console_closePacket(uint8_t cmdID)
{
    console.putc(0x7E); // start of frame flag
    console.putc(cmdID);
    console.putc(packetIndex);
    int sum = 0x7E + cmdID + packetIndex;
    for (unsigned i=0; i<packetIndex; i++)
    {
        uint8_t c = console_packet_buffer[i];
        sum += c;
        console.putc(c);
    }
    console.putc(sum & 0xFF);
    console.putc((sum >> 8) & 0xFF);
    console.putc(0x7E); // end of frame flag
    packetIndex = 0;
}

//------------------------------------------------------------------------------
void console_appendPacket8(uint8_t b)
{
    console_packet_buffer[packetIndex++] = b;
}

//------------------------------------------------------------------------------
void console_appendPacket16(uint16_t b)
{
    console_packet_buffer[packetIndex++] = (b & 0xFF);
    console_packet_buffer[packetIndex++] = (b >> 8) & 0xFF;
}

static void emitChar(int ch)
{
    console_packet_buffer[packetIndex++] = ch;
}
//------------------------------------------------------------------------------
int xprintf(const char* fmt, ...) {
    va_list ap; 
    va_start(ap, fmt); 
    console_openPacket();
    veprintf(emitChar, fmt, ap); va_end(ap);
    console_closePacket(0xFF);
    return 0;
}

