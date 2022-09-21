#include "config.h"
#include "_my_config.h"

#if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)

#include "rx.h"
#include "rx_afhds2a/afhds2a.h"
#include "rx_afhds2a/afhds2a_defs.h"    // for FLYSKY_2A_PAYLOAD_SIZE
#include "console.h"

extern "C" {
#include "util.h"
}

//------------------------------------------------------------------------------
// Globals referenced by code outside of this module
extern "C" {

extern uint32_t gettime();

#if defined(RX_FLYSKY) 
int packet_period = 1500;
#elif defined(RX_FLYSKY2A)
int packet_period = 3850;
#else
    #error
#endif

int packetpersecond;
int pkt_hits;
int b_crc_errors;
int hw_crc_errors;	// packet (hardware or simulated hardware) crc error

// rx.c
bool failsafe = true;
float rx[ 4 ];
char aux[ AUXNUMBER ];
float aux_analog[ 2 ] = { 1.0, 1.0 };

// referenced by flash.c
//char rfchannel[ 4 ];
//uint8_t rxaddress[ 5 ];
//int telemetry_enabled = 0;
//int rx_bind_load = 0;


int rxmode = RXMODE_BIND;   // RXMODE_BIND == 0;

}

//------------------------------------------------------------------------------
static uint8_t payload[FLYSKY_2A_PAYLOAD_SIZE];
static uint16_t rcData[FLYSKY_2A_CHANNEL_COUNT];    // rc channels
static uint32_t secondtimer;
static uint16_t packetrx;
static uint32_t lastRXTime;

//------------------------------------------------------------------------------
// Define only one of the following
//#define FLYSKY_i6_MAPPING
#define TURNIGY_EVOLUTION_MAPPING

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
    kIBUS_Aux3,     // SwB          SwC/RightSw(1-3)    // 1000, 1500, 2000
    kIBUS_Aux4,     // SwC(1-3)     VrA(Rotary)         // 1000, 2000
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
    { THROTTLE_KILL_SWITCH,     kIBUS_Aux1,     0,  50  },
#if defined(LEVELMODE)    
    { LEVELMODE,                kIBUS_Aux2,     0,  50  },
#endif    
    { MOTOR_BEEPS_CHANNEL,      kIBUS_Aux3,     25, 65 },
#if defined(TURTLE_MODE)    
    { TURTLE_MODE,              kIBUS_Aux3,     75, 100 },
#endif    
    { RATES,                    kIBUS_Aux4,     0,  50 }
};
#endif

// Turnigy Evolution
//  Feature                 Turnigy Evolution
//  -------                 ---------
//  THROTTLE_KILL_SWITCH    SwB/1   kIBUS_Aux1
//  LEVELMODE               SwB/2   kIBUS_Aux2
//  MOTOR_BEEPS_CHANNEL     SwC/2   kIBUS_Aux3
//  RATES                   SwC/3   kIBUS_Aux3
//  TURTLE_MODE             SwA/2   kIBUS_Aux4
//    kIBUS_Aux1,     // VrA          SwB/LeftSw(1-3)     // 1000, 1500, 2000
//    kIBUS_Aux2,     // VrB          SwA/MidSw(1-2)      // 1000(dn), 2000(up)
//    kIBUS_Aux3,     // SwB          SwC/RightSw(1-3)    // 1000, 1500, 2000
//    kIBUS_Aux4,     // SwC(1-3)     VrA(Rotary)         // 1000, 2000
#ifdef TURNIGY_EVOLUTION_MAPPING
static uint8_t aux_map[][4] =
{
    { THROTTLE_KILL_SWITCH,     kIBUS_Aux1,     0,  30  },
#if defined(LEVELMODE)    
    { LEVELMODE,                kIBUS_Aux2,     35, 75  },
#endif    
    { MOTOR_BEEPS_CHANNEL,      kIBUS_Aux3,     35, 75  },
    { RATES,                    kIBUS_Aux3,     0,  30  },
#if defined(TURTLE_MODE)    
    { TURTLE_MODE,              kIBUS_Aux4,     35, 100 },
#endif    
};
#endif


//------------------------------------------------------------------------------
// Channel values are in the range of 1000 to 2000 inclusive (but can be as low 
// as 988 and high as 2020). 
// So we convert to range -1.0f to +1.0f and clamp as necessary
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
// Channel values are in the range of 1000 to 2000 inclusive (but can be as low 
// as 988 and high as 2020). 
// So we convert to range 0.0f to +1.0f and clamp as necessary
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
void rx_init( void )
{
    rxSpiConfig_t rxConfig;
#if defined(RX_FLYSKY2A)
    rxConfig.rx_spi_protocol = RX_SPI_A7105_FLYSKY_2A;
#else    
    rxConfig.rx_spi_protocol = RX_SPI_A7105_FLYSKY;
#endif    
    rxConfig.extiIoTag = 0;

    rxRuntimeConfig_t rxRuntimeConfig;  // this isn't read, only written to
    flySkyInit(&rxConfig, &rxRuntimeConfig);
}

//------------------------------------------------------------------------------
void checkrx( void )
{
    uint32_t now = gettime();

    rx_spi_received_e status = flySkyDataReceived(payload);
    switch (status)
    {
        case RX_SPI_RECEIVED_NONE:
            break;

        // If we've just bound
        case RX_SPI_RECEIVED_BIND:
        {
            rxmode = RXMODE_NORMAL;
        }
        break;

        // If packet was received
        case RX_SPI_RECEIVED_DATA:
        {
            rxmode = RXMODE_NORMAL;
            flySkySetRcDataFromPayload(rcData, payload);

            // Channels are in the range of 1000 to 2000 inclusive (but can be as low as 988 and high as 2020)
            // So we convert to range -1.0f to +1.0f and clamp as necessary
            // Except for THROTTLE which we convert to range of 0.0f to 1.0f
            rx[0] = AER_to_Neg1_to_Pos1(rcData[0]);   // Aileron / Roll
            rx[1] = AER_to_Neg1_to_Pos1(rcData[1]);   // Elevator / Pitch
            rx[2] = AER_to_Neg1_to_Pos1(rcData[3]);   // Rudder / Yaw
            rx[3] = T_to_0_to_1(rcData[2]);

            // Apply expo
#ifdef LEVELMODE
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


            // Aux channels
            const int kNumMapped = sizeof(aux_map) / sizeof(aux_map[0]);
            for (int i=0; i<kNumMapped; i++)
            {
                const uint8_t *mapping = aux_map[i];
                const uint8_t internal_aux_index = mapping[0];
                const uint8_t ibus_aux_index = mapping[1];

                // Map IBUS channel value from range 1000...2000 to 0..100
                uint16_t chVal = rcData[ibus_aux_index];
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

            packetrx++;
            lastRXTime = now;
            failsafe = false;

#if 0
            static uint32_t last;
            uint32_t now = gettime();
            if ((now - last) > (100000 * 4))
            {
            #if 1
                // log IBUS AUX1 thru AUX6
                console_sendPacket(3, (uint8_t*)(rcData+4), 6*2);
            #else
                // Log AETR, AUX1, AUX2
                console_sendPacket(3, (uint8_t*)(rcData+0), 6*2);
            #endif        
                last = now;
            }
#endif

        }
        break;
    }

    // Check for failsafe
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

    // Some stats we update every second
	if ( now - secondtimer > 1000000 ) {
		packetpersecond = packetrx;
        pkt_hits = packetrx;
        packetrx = 0;  
		secondtimer = now;
	}
}

#endif // #if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)
