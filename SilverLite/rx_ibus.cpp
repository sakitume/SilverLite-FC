#include "jee.h"
#include "rx.h"
#include "config.h"
#include "console.h"

extern "C" {
#include "util.h"
}

#if defined(RX_IBUS)

static uint8_t ibus_aux_mapping[6] =
{
    0,
    1,
    2,
    3,
    4,
    5
};

//------------------------------------------------------------------------------
extern "C" {

extern uint32_t gettime();

int packetpersecond;
int pkt_hits;
int b_crc_errors;
int hw_crc_errors;	// packet (hardware or simulated hardware) crc error

// rx.c
bool failsafe = true;
float rx[ 4 ];
char aux[ AUXNUMBER ];
float aux_analog[ 2 ] = { 1.0, 1.0 };

char rfchannel[ 4 ];
uint8_t rxaddress[ 5 ];
int telemetry_enabled = 0;

int rx_bind_load = 0;
int rxmode = 0;

}

//------------------------------------------------------------------------------
static uint32_t secondtimer;
static uint16_t crc_errors;
static uint16_t packetrx;
static uint16_t packet_hits;

//------------------------------------------------------------------------------
// Need an interrupt driven receiver with a buffer of at least 8 bytes
// or we will lose bytes
#if defined(STM32F405xx)    // OMNIBUSF4
static UartBufDev< PinA<9>, PinA<10>, 16 > uart;    // USART1
//static UartDev< PinA<9>, PinA<10> > uart;         // USART1
#elif defined(STM32F411xE)  // NOX
static UartBufDev< PinA<2>, PinA<3>, 32 > uart;     // USART2
#endif

//------------------------------------------------------------------------------
// Based on: https://github.com/33d/ibus-library/blob/master/ibus.c
/*
  The data is serial data, 115200, 8N1.
  Messages arrive every 7 milliseconds, and are read constantly until a
  few tenths of a second after the transmitter is switched off.
  Packet format:
  20 40 CH0 CH1 CH2 CH3 CH4 CH5 CH6 CH7 CH8 CH9 CH10 CH11 CH12 CH13 SUM
  Channels are stored in little endian byte order.  Unused channels read
  5DC (first byte DC, second byte 05).
  Channel 0: Right horizontal: 3E8 -> 7D0
  Channel 1: Right vertical:   3E8 -> 7CF
  Channel 2: Left vertical:    3E8 -> 7D0
  Channel 3: Left horizontal:  3E8 -> 7CD
  Channel 4: Left pot:         3E8 -> 7D0
  Channel 5: Right pot:        3E8 -> 7D0
  The checksum starts at 0xFFFF, then subtract each byte except the 
  checksum bytes.
*/

struct ibus_state {
    uint_fast8_t    state;          // The current byte index of the incoming IBUS packet/frame
    uint_fast16_t   checksum;
    uint_fast8_t    datal;
    uint16_t        channels[14];
};
static ibus_state state;

//------------------------------------------------------------------------------
void rx_init()
{
    state.state = 0;

    uart.init();

    // TODO, XXX: A bug exists somewhere that causes the baud rate of
    // USART1 (when tested on OMNIBUSF4 board) to be incorrectly set.
    // I've discovered I can double the requested baudrate and get it 
    // to work correctly. I'll investigate further but will punt for now.
#ifdef STM32F405xx
    int hz = 168000000;      // OMNIBUSF4 uses F405 running at 168
#elif defined STM32F411xE
    int hz = 96000000;       //NOX/NOXE uses F411 running at 96
#else
    #error "Unable to determine what F4 variant to target"
#endif    
    uart.baud(115200 * 2, hz);
}

//------------------------------------------------------------------------------
static void handlePacket(const uint16_t *channels)
{
#if 0    
    static uint32_t last;
    uint32_t now = gettime();
    if ((now - last) > (100000 * 4))
    {
        console_sendPacket(3, (uint8_t*)channels, 14*2);
        last = now;
    }
#endif

    // Channels are in the range of 1000 to 2000 inclusive (but can be as low as 988 and high as 2020)
    // So we convert to range -1.0f to +1.0f and clamp as necessary
    for (int i=0; i<4; i++)
    {
        int v = channels[i] - 1500;
        if (v < -500)
        {
            v = -500;
        }
        else if (v > 500)
        {
            v = 500;
        }
        rx[i] = v * 0.002f;
    }

#ifdef LEVELMODE
    // level mode expo
    if (aux[ LEVELMODE ])
    { 
        if (ANGLE_EXPO_ROLL > 0.01) 	rx[0] = rcexpo(rx[0], ANGLE_EXPO_ROLL);
        if (ANGLE_EXPO_PITCH > 0.01) 	rx[1] = rcexpo(rx[1], ANGLE_EXPO_PITCH);
        if (ANGLE_EXPO_YAW > 0.01) 		rx[2] = rcexpo(rx[2], ANGLE_EXPO_YAW);
    }
    else
#endif			
    {
        if (ACRO_EXPO_ROLL > 0.01) 	rx[0] = rcexpo(rx[0], ACRO_EXPO_ROLL);
        if (ACRO_EXPO_PITCH > 0.01)	rx[1] = rcexpo(rx[1], ACRO_EXPO_PITCH);
        if (ACRO_EXPO_YAW > 0.01) 	rx[2] = rcexpo(rx[2], ACRO_EXPO_YAW);
    }

    for (int i=0; i<6; i++)
    {
        aux[i] = (channels[4 + ibus_aux_mapping[i]] > 1600) ? 1 : 0;
    }
}

//------------------------------------------------------------------------------
void checkrx()
{
#if 0
    static int cntr;
    static uint8_t buff[32];
    while (uart.readable())
    {
        buff[cntr++] = uart.getc();
        if (cntr >= 1)
        {
            cntr = 0;
            console_sendPacket(3, buff, 1);
        }
    }
#else    
    while (uart.readable())
    {
        uint8_t ch = uart.getc();
        switch (state.state)
        {
            // Waiting to hit 1st byte of packet
            case 0:
                if (ch == 0x20)
                {
                    state.checksum = 0xFFFF - 0x20;
                    state.state = 1;
                }
                break;

            // Second byte should be 0x40
            case 1:
                if (ch == 0x40)
                {
                    state.state = 2;
                    state.checksum -= ch;
                }
                else // ...hmmm, this wasn't the second byte of a packet!
                {
                    state.state = 0;    // wait for 1st byte
                }
                break;

            // Byte 30 is low byte of 16bit checksum
            case 30:
                state.datal = ch;
                state.state = 31;
                break;

            // Byte 31 is high byte of 16bit checksum (and also end of the packet/frame)
            case 31:
            {
                packet_hits++;
                uint_fast16_t checksum = (ch << 8) | state.datal;
                state.state = 0;
                if (checksum == state.checksum)
                {
                    handlePacket(state.channels);
                    packetrx++;
                }
                else
                {
                    crc_errors++;
                }
            }
            break;

            // Received a byte of a 16bit channel value
            default:
                if ((state.state & 1) == 0)
                {
                    // ...was the low byte
                    state.datal = ch;
                }
                else
                {
                    // ...was the high byte
                    state.channels[(state.state / 2) - 1] = (ch << 8) | state.datal;
                }
                state.checksum -= ch;
                state.state++;
                break;
        }
    }

	if ( gettime() - secondtimer > 1000000 ) {
		b_crc_errors = crc_errors;
		crc_errors = 0;
		packetpersecond = packetrx;
		packetrx = 0;
        pkt_hits = packet_hits;
        packet_hits = 0;
		secondtimer = gettime();
	}
#endif    
}

#endif // #if defined(RX_IBUS)
