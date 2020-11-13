#include "jee.h"
#include "rx.h"
#include "config.h"
#include "console.h"

extern "C" {
#include "util.h"
}

#if defined(RX_IBUS)


//------------------------------------------------------------------------------
// Define only one of the following
#define FLYSKY_i6_MAPPING
//#define TURNIGY_EVOLUTION_MAPPING



//------------------------------------------------------------------------------
// IBUS supports 14 channels but in reality our transmitters and receivers
// only ever support 10 channels (unless you use custom firmware).
// The first 4 channels are AETR (Aileron, Elevator, Thrust, Rudder)
// corresponding to Roll, Pitch, Throttle, Yaw. The remaining 6 channels
// can be considered auxiliary channels.
//
// Channel values are (nominally) between 1000 to 2000. Although they can be
// a little below 1000 or a little above 2000.
//
// Below is an enumeration list of constants to describe the IBUS channels.
// The comments after each aux channel describe which switch/output of the
// FlySky FS-i6 my SilverLite firmware uses for that channel. For switches
// the "Up" position of a switch corresponds to a low value of 1000
// while the "Down" position corresponds to a high value of 2000
enum e_IBUSChannels
{
    kIBUS_A,
    kIBUS_E,
    kIBUS_T,
    kIBUS_R,

                    // FlySky i6    Turnigy Evolution
    kIBUS_Aux1,     // VrA          SwB/LeftSw(1-3)     // 1000, 1500, 2000
    kIBUS_Aux2,     // VrB          SwA/MidSw(1-2)      // 1000(up), 2000(dn)
    kIBUS_Aux3,     // SwB          VrA(Rotary)         // 1000, 2000
    kIBUS_Aux4,     // SwC(1-3)     SwC/RightSw(1-3)    // 1000, 1500, 2000
    kIBUS_Aux5,     // SwA(1-2)
    kIBUS_Aux6,     // SwD(1-2)
};

//------------------------------------------------------------------------------
// The following array is used to map an IBUS aux channel value to one of our
// internal aux[] boolean. Each entry of the array consists of 4 values:
//  * aux[] channel to set/clear (See header file "defines.h")
//  * IBUS aux channel index (kIBUS_Aux1...kIBUS_Aux6)
//  * min value
//  * max value 
//
// An IBUS channel value is normalized to the range of 0..100 (inclusive) and
// if its value is between the min/max values (inclusive) then the corresponding
// aux[] channel is set, otherwise it is cleared
//
// Internal aux[] channels are used to switch on/off various features. Examine
// _my_config.h to see how those features are tied to aux channels. Below
// is the configuration I generally use.
//
//  Feature                 FlySky i6
//  -------                 ---------
//  THROTTLE_KILL_SWITCH    SwA/1
//  LEVELMODE               SwB/1
//  MOTOR_BEEPS_CHANNEL     SwC/3
//  RATES                   SwD/1
//
#ifdef FLYSKY_i6_MAPPING
static uint8_t aux_map[][4] =
{
    { THROTTLE_KILL_SWITCH,     kIBUS_Aux5,     0,  50  },
    { LEVELMODE,                kIBUS_Aux3,     0,  50  },
    { MOTOR_BEEPS_CHANNEL,      kIBUS_Aux4,     25, 100 },
    { RATES,                    kIBUS_Aux6,     0,  50 }
};
#endif

// Turnigy Evolution
//  Feature                 Turnigy Evolution
//  -------                 ---------
//  THROTTLE_KILL_SWITCH    SwB/1   kIBUS_Aux1
//  LEVELMODE               SwB/2   kIBUS_Aux1
//  MOTOR_BEEPS_CHANNEL     SwA/2   kIBUS_Aux3
//  RATES                   SwC/1   kIBUS_Aux4
//    kIBUS_Aux1,     // VrA          SwB/LeftSw(1-3)     // 1000, 1500, 2000
//    kIBUS_Aux2,     // VrB          SwA/MidSw(1-2)      // 1000(up), 2000(dn)
//    kIBUS_Aux3,     // SwB          VrA(Rotary)         // 1000, 2000
//    kIBUS_Aux4,     // SwC(1-3)     SwC/RightSw(1-3)    // 1000, 1500, 2000
#ifdef TURNIGY_EVOLUTION_MAPPING
static uint8_t aux_map[][4] =
{
    { THROTTLE_KILL_SWITCH,     kIBUS_Aux1,     0,  30  },
    { LEVELMODE,                kIBUS_Aux1,     35, 75  },
    { MOTOR_BEEPS_CHANNEL,      kIBUS_Aux2,     50, 100 },
    { RATES,                    kIBUS_Aux4,     0,  35  },
};
#endif


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
static uint32_t lastRXTime;

//------------------------------------------------------------------------------
// Need an interrupt driven receiver with a buffer of at least 8 bytes
// or we will lose bytes
#if defined(OMNIBUSF4)
static UartBufDev< PinA<9>, PinA<10>, 16 > uart;    // USART1
#elif defined(NOX)  // NOX
    //#define __NOX_UART2__
    #ifdef __NOX_UART2__
    static UartBufDev< PinA<2>, PinA<3>, 16 > uart;     // USART2 (SBUS pad on NOX board)
    static Pin<'C', 14>		INVERTER_PIN;	            // PC14
    #else
    static UartBufDev< PinB<6>, PinB<7>, 16 > uart;     // USART1 (R1 on center of board, backside. T1 on top/right center of board, topside)
    #endif
#elif defined(MATEKF411RX)
static UartBufDev< PinA<9>, PinA<10>, 16 > uart;        // USART1
#else
    #error "Pin assignements must be defined for this target"
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
    uart.baud(115200 * 2, hz);
#elif defined STM32F411xE
    int hz = 96000000;       //NOX/NOXE uses F411 running at 96
    uart.baud(115200, hz);

    #ifdef __NOX_UART2__
    INVERTER_PIN.mode(Pinmode::out);
    INVERTER_PIN.write(1);
    #endif    
#else
    #error "Unable to determine what F4 variant to target"
#endif    
}

//------------------------------------------------------------------------------
static float AER_to_Neg1_to_Pos1(int v)
{
     v -= 1500;
    if (v < -500)
    {
        v = -500;
    }
    else if (v > 500)
    {
        v = 500;
    }
    return v * 0.002f;
}

//------------------------------------------------------------------------------
static float T_to_0_to_1(int v)
{
     v -= 1000;
    if (v < 0)
    {
        v = 0;
    }
    else if (v > 1000)
    {
        v = 1000;
    }
    return v * 0.001f;
}

//------------------------------------------------------------------------------
static void handlePacket(const uint16_t *channels)
{
#if 0
    static uint32_t last;
    uint32_t now = gettime();
    if ((now - last) > (100000 * 4))
    {
    #if 0        
        // log IBUS AUX1 thru AUX6
        console_sendPacket(3, (uint8_t*)(channels+4), 6*2);
    #else
        // Log AETR, AUX1, AUX2
        console_sendPacket(3, (uint8_t*)(channels+0), 6*2);
    #endif        
        last = now;
    }
#endif

#if 0
    // log rx[] (Roll, Pitch, Yaw and Throttle)
    static uint32_t last;
    uint32_t now = gettime();
    if ((now - last) > (100000 * 4))
    {
        console_sendPacket(3, (uint8_t*)rx, 4*sizeof(rx[0]));
        last = now;
    }
#endif

    // Channels are in the range of 1000 to 2000 inclusive (but can be as low as 988 and high as 2020)
    // So we convert to range -1.0f to +1.0f and clamp as necessary
    // Except for THROTTLE which we convert to range of 0.0f to 1.0f
    rx[0] = AER_to_Neg1_to_Pos1(channels[0]);   // Aileron / Roll
    rx[1] = AER_to_Neg1_to_Pos1(channels[1]);   // Elevator / Pitch
    rx[2] = AER_to_Neg1_to_Pos1(channels[3]);   // Rudder / Yaw
    rx[3] = T_to_0_to_1(channels[2]);

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

    const int kNumMapped = sizeof(aux_map) / sizeof(aux_map[0]);
    for (int i=0; i<kNumMapped; i++)
    {
        const uint8_t *mapping = aux_map[i];
        const uint8_t internal_aux_index = mapping[0];
        const uint8_t ibus_aux_index = mapping[1];

        // Map IBUS channel value from range 1000...2000 to 0..100
        uint16_t chVal = channels[ibus_aux_index];
        if (chVal <= 1000)
        {
            chVal = 0;
        }
        else if (chVal >= 2000)
        {
            chVal = 100;
        }
        else
        {
            chVal = (chVal - 1000) / 10;
        }

        // Set internal aux[] boolean if IBUS channel value fell within min/max, otherwise clear the boolean
        aux[internal_aux_index] = ((chVal >= mapping[2]) && (chVal <= mapping[3]));
    }
}

//------------------------------------------------------------------------------
void checkrx()
{
    uint32_t now = gettime();

#if 0   // Show state of aux[] channels that we manage
    static uint32_t emit_timer;
	if ( now - emit_timer > 200000 )
    {
        emit_timer = now;
        static uint8_t buff[10];

        const int kNumMapped = sizeof(aux_map) / sizeof(aux_map[0]);
        for (int i=0; i<kNumMapped; i++)
        {
            const uint8_t *mapping = aux_map[i];
            const uint8_t internal_aux_index = mapping[0];
            buff[i] = aux[internal_aux_index];
        }
        console_sendPacket(3, buff, kNumMapped);
    }
#endif

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

                    lastRXTime = now;
                    failsafe = false;
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

    if ((rxmode == RXMODE_NORMAL) && !failsafe)
    {
        if ((now - lastRXTime) >= FAILSAFETIME)
        {
            failsafe = true;
            for ( int i = 0; i < 4; ++i ) 
            {
                rx[ i ] = 0.0f;
            }
        }
    }

	if ( now - secondtimer > 1000000 ) {
		b_crc_errors = crc_errors;
		crc_errors = 0;
		packetpersecond = packetrx;
		packetrx = 0;
        pkt_hits = packet_hits;
        packet_hits = 0;
		secondtimer = now;
	}
}

#endif // #if defined(RX_IBUS)
