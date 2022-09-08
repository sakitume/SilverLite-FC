#include "drv_serial_rx.h"
#include "rx.h"
#include "config.h"
#include "console.h"
#include <string.h>

#include "_my_config.h"
#include <stdint.h>

#if defined(STM32F4)
    #include "stm32f4xx.h"
    #include "stm32f4xx_ll_gpio.h"
    #include "stm32f4xx_ll_rcc.h"
    #include "stm32f4xx_ll_bus.h"
    #include "stm32f4xx_ll_usart.h"
#endif

#if defined(STM32F3)
    #include "stm32f3xx.h"
    #include "stm32f3xx_ll_gpio.h"
    #include "stm32f3xx_ll_bus.h"
    #include "stm32f3xx_ll_usart.h"
#endif

//------------------------------------------------------------------------------
// SilF4ware header files that were meant for C, not C++ need to be wrapped
extern "C" {
#include "util.h"
#include "drv_time.h"
}

//------------------------------------------------------------------------------
#if defined(RX_ELRS)

//------------------------------------------------------------------------------
extern "C" {

extern uint32_t gettime();

int packet_period = 4000;   // Default to 250 packets per second
int packetpersecond;        // ref'd by update_osd()

// Referenced by numerous sources
bool failsafe = true;
float rx[ 4 ];
char aux[ AUXNUMBER ];
float aux_analog[ 2 ] = { 1.0, 1.0 };
int rxmode = 0;

}

//------------------------------------------------------------------------------
// See: ExpressLRS/src/lib/CrsfProtocol/crsf_protocol.h
#define CRSF_FRAME_SIZE_MAX 64
#define CRSF_SYNC_BYTE 0xC8

typedef struct crsfPayloadLinkstatistics_s
{
    uint8_t uplink_RSSI_1;
    uint8_t uplink_RSSI_2;
    uint8_t uplink_Link_quality;
    int8_t uplink_SNR;
    uint8_t active_antenna;
    uint8_t rf_Mode;
    uint8_t uplink_TX_Power;
    uint8_t downlink_RSSI;
    uint8_t downlink_Link_quality;
    int8_t downlink_SNR;
} crsfLinkStatistics_t;

// Frame type
enum
{
    CRSF_FRAMETYPE_LINK_STATISTICS          = 0x14, // 0x14
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED       = 0x16, // 0x16
};

// See: include/common.h in ExpressLRS repo
// Note: I'm using ELRS 3.0 release candidate (not ELRS2) source for reference
typedef enum : uint8_t
{
    RATE_LORA_4HZ = 0,
    RATE_LORA_25HZ,
    RATE_LORA_50HZ,
    RATE_LORA_100HZ,
    RATE_LORA_100HZ_8CH,
    RATE_LORA_150HZ,
    RATE_LORA_200HZ,
    RATE_LORA_250HZ,
    RATE_LORA_333HZ_8CH,
    RATE_LORA_500HZ,
    RATE_DVDA_250HZ,
    RATE_DVDA_500HZ,
    RATE_FLRC_500HZ,
    RATE_FLRC_1000HZ,
} expresslrs_RFrates_e; // Max value of 16 since only 4 bits have been assigned in the sync package.

static uint32_t ELRSPacketPeriods[] = 
{
    1000000 / 4,    // RATE_LORA_4HZ = 0,
    1000000 / 25,   // RATE_LORA_25HZ,
    1000000 / 50,   // RATE_LORA_50HZ,
    1000000 / 100,  // RATE_LORA_100HZ,
    1000000 / 100,  // RATE_LORA_100HZ_8CH,
    1000000 / 150,  // RATE_LORA_150HZ,
    1000000 / 200,  // RATE_LORA_200HZ,
    1000000 / 250,  // RATE_LORA_250HZ,
    1000000 / 333,  // RATE_LORA_333HZ_8CH,
    1000000 / 500,  // RATE_LORA_500HZ,
    1000000 / 250,  // RATE_DVDA_250HZ,
    1000000 / 500,  // RATE_DVDA_500HZ,
    1000000 / 500,  // RATE_FLRC_500HZ,
    1000000 / 1000, // RATE_FLRC_1000HZ,
};


//------------------------------------------------------------------------------
// See: https://github.com/ExpressLRS/ExpressLRS/wiki/CRSF-Protocol
// CRSF format channel data. 16 channels, 11 bits per channel
// CRSF channel values (0-1984). 
//  Min - 172 represents 988us
//  Mid - CRSF 992 represents 1500us
//  Max - 1811 represents 2012us.
struct CRSFChannelData
{
    uint32_t chan0 : 11;
    uint32_t chan1 : 11;
    uint32_t chan2 : 11;
    uint32_t chan3 : 11;
    uint32_t chan4 : 11;
    uint32_t chan5 : 11;
    uint32_t chan6 : 11;
    uint32_t chan7 : 11;
    uint32_t chan8 : 11;
    uint32_t chan9 : 11;
    uint32_t chan10 : 11;
    uint32_t chan11 : 11;
    uint32_t chan12 : 11;
    uint32_t chan13 : 11;
    uint32_t chan14 : 11;
    uint32_t chan15 : 11;
} __attribute__((__packed__));

//------------------------------------------------------------------------------
// crc implementation from CRSF protocol document rev7
static const uint8_t crsf_crc8tab[256] = 
{
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

//------------------------------------------------------------------------------
static uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        crc = crsf_crc8tab[crc ^ *ptr++];
    }
    return crc;
}

//------------------------------------------------------------------------------
// Represents an incoming frame and provides methods to process that frame.
struct SerialFrame
{
    enum
    {
        kRxBuffSize   = CRSF_FRAME_SIZE_MAX*2,
        kFramesRingBufferCount  = 2
    };

    uint8_t     m_rxBuffer[kRxBuffSize];    // serial receive buffer
    volatile int8_t m_frameHead;            // ring buffer head (write pointer)
    volatile int8_t m_frameTail;            // ring buffer tail (read pointer)
    uint8_t     m_frames[kFramesRingBufferCount][CRSF_FRAME_SIZE_MAX+2];   // ring buffer of frames, need 2 extra bytes per frame for type and crc bytes
    uint8_t     m_rxBufferCount;

    uint16_t    m_processedFrames;          // number of frames that have been processed (reset every second)
    uint32_t    m_lastRxTime;               // timestamp for when last frame was processed
    uint8_t     m_rfMode;

    void    Update();
    void    ProcessFrame(const uint8_t* pFrame);
    void    RxIRQHandler(USART_TypeDef* usart);
};
static SerialFrame gSerialFrame;

//------------------------------------------------------------------------------
// This is our serial RX interrupt request handler
void SerialFrame::RxIRQHandler(USART_TypeDef* usart)
{
    // Measure time interval between now and last byte we received
    static uint32_t lastTime;
    uint32_t now = gettime();
    uint32_t delta = now - lastTime;
    lastTime = now;

    // If the amount of time since the last byte was received is more than
    // the minimum time betwen frames (or we've finished with the last frame)
    // then we'll consider this incoming byte as the start of a new frame
    const uint32_t kMinTimeBetweenFrames = 250; 
    if (delta > kMinTimeBetweenFrames)
    {
        m_rxBufferCount = 0;
    }

    // If overrun error (lost data)
    if (LL_USART_IsActiveFlag_ORE(usart))
    {
        LL_USART_ClearFlag_ORE(usart);
        m_rxBufferCount = 0;
    }

    if (LL_USART_IsActiveFlag_RXNE(usart)) 
    {
        uint8_t data = LL_USART_ReceiveData8(usart);

        // If waiting for start of frame
        if (m_rxBufferCount == 0)
        {
            if (data != CRSF_SYNC_BYTE)
            {
//XXX                m_rxErrorSync++;
            }
            else
            {
                m_rxBuffer[m_rxBufferCount++] = data;
            }
            return;
        }

        // If this incoming byte is the "Frame length" byte and it's value is out of range
        // then fail and reset our m_rxBuffer
        if ((m_rxBufferCount == 1) && ((data < 2) || (data > CRSF_FRAME_SIZE_MAX)))
        {
            m_rxBufferCount = 0;
            return;
        }

        // If there is room in m_rxBuffer then append this incoming byte into it
        if (m_rxBufferCount < kRxBuffSize)
        {
            m_rxBuffer[m_rxBufferCount++] = data;
        } 
        else // ...otherwise fail and reset the buffer
        {
            m_rxBufferCount = 0;
//XXX            m_rxErrorOverflow++;
            return;
        }

        // If we have the minimum number of bytes of a frame in our rx buffer
        if (m_rxBufferCount > 4) 
        {
            // If this is the final byte of the frame
            uint8_t length = m_rxBuffer[1];
            if (length + 2 == m_rxBufferCount)   // +2 because, +1 for type byte, +1 for crc byte
            {
                // Copy the received frame into next free slot in our frames ring buffer
                // Note: We do not check crc at this time and instead leave that for main 
                // thread (our Update() method) to perfrom that validation. To keep this IRQ
                // handler as fast as possible
                const int i = (m_frameHead + 1) % kFramesRingBufferCount;
                if (i != m_frameTail) 
                {
                    uint8_t* pFrame = m_frames[m_frameHead];
                    memcpy(pFrame, m_rxBuffer, m_rxBufferCount);
                    m_frameHead = i;
                }
                else
                {
                    // Our ring buffer of frames is full, main thread isn't keeping up with us
                    // for some reason. Nothing we can do about that....this frame will be dropped
                }

                m_rxBufferCount = 0;
            }
        }
    }
}


//#define USE_LOG_CHANNEL_PACKET
#if defined(USE_LOG_CHANNEL_PACKET)
//------------------------------------------------------------------------------
static void LogChannelPacket(const int32_t *channels)
{
#if 1
    static uint32_t last;
    uint32_t now = gettime();
    if ((now - last) > (100000 * 4))
    {
        // log 4 channels, from channels[4]
        console_sendPacket(6, (uint8_t*)(&channels[4]), 4*4);
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
}
#endif

//------------------------------------------------------------------------------
// This is called when we've successfully parsed a frame of ELRS/CRSF data from
// our incoming serial byte stream
void SerialFrame::ProcessFrame(const uint8_t* pFrame)
{
    rxmode = RXMODE_NORMAL;
    m_processedFrames++;
    m_lastRxTime = gettime();
    failsafe = false;

    switch (pFrame[2])
    {
        case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
        {
            const CRSFChannelData* chan = (CRSFChannelData*)&pFrame[3];
            static int32_t channels[16];
            channels[0] = chan->chan0;
            channels[1] = chan->chan1;
            channels[2] = chan->chan2;
            channels[3] = chan->chan3;
            channels[4] = chan->chan4;
            channels[5] = chan->chan5;
            channels[6] = chan->chan6;
            channels[7] = chan->chan7;
            channels[8] = chan->chan8;
            channels[9] = chan->chan9;
            channels[10] = chan->chan10;
            channels[11] = chan->chan11;
            channels[12] = chan->chan12;
            channels[13] = chan->chan13;
            channels[14] = chan->chan14;
            channels[15] = chan->chan15;

            // AETR channel order
            rx[0] = (channels[0] - 990.5f) * 0.00125707103f;
            rx[1] = (channels[1] - 990.5f) * 0.00125707103f;
            rx[2] = (channels[3] - 990.5f) * 0.00125707103f;
            rx[3] = (channels[2] - 191.0f) * 0.00062853551f;
            if ( rx[3] > 1 ) rx[3] = 1;	
            if ( rx[3] < 0 ) rx[3] = 0;

            // Apply expo
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

            // Update aux channels
            aux[THROTTLE_KILL_SWITCH]   = (channels[4] > 880) ? 0 : 1;      // 
            aux[MOTOR_BEEPS_CHANNEL]    = (channels[5] > 1100) ? 1 : 0;
            aux[LEVELMODE]              = 0;

#if defined(USE_LOG_CHANNEL_PACKET)
            LogChannelPacket(channels);
#endif            
        }
        break;

        case CRSF_FRAMETYPE_LINK_STATISTICS:
        {
            // From the stats data we can determine the ELRS packet rate and use that to
            // update the global 'packet_period' which is used by the rx smoothing code
            const crsfPayloadLinkstatistics_s* stats = (crsfPayloadLinkstatistics_s* )&pFrame[3];
            m_rfMode = stats->rf_Mode;
            if ((m_rfMode >= RATE_LORA_4HZ) && (m_rfMode <= RATE_FLRC_1000HZ))
            {
                packet_period = ELRSPacketPeriods[m_rfMode];
            }

            // TODO: Rather than displaying packets per second in OSD we could
            // instead display stats->uplink_Link_quality
        }
        break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------
// This is called once per loop iteration by main thread
void SerialFrame::Update()
{
    // Check if there is a new frame available
    if (m_frameTail != m_frameHead)
    {
        const uint8_t* pFrame = m_frames[m_frameTail];
        m_frameTail = (uint8_t)(m_frameTail + 1) % kFramesRingBufferCount;

        const uint8_t length = pFrame[1];
        const uint8_t crc = crsf_crc8(pFrame + 2, length - 1);
        if (crc != pFrame[length + 1])
        {
            // crc mismatch
        }
        else
        {
            ProcessFrame(pFrame);
        }
    }

    // Check for failsafe
    if ((rxmode == RXMODE_NORMAL) && !failsafe)
    {
        if ((gettime() - m_lastRxTime) >= FAILSAFETIME)
        {
            failsafe = true;
            for ( int i = 0; i < 4; ++i ) 
            {
                rx[ i ] = 0.0f;
            }
        }
    }

    // Update our packetpersecond stat (used by OSD) if 1 second has passed
    static uint32_t secondtimer;
    uint32_t now = gettime();    
	if ( now - secondtimer >= 1000000 ) 
    {
		packetpersecond = m_processedFrames;
		m_processedFrames = 0;
		secondtimer = now;
	}
}


//------------------------------------------------------------------------------
static void RxIRQHandler(void* _usart)
{
    USART_TypeDef* usart = (USART_TypeDef*)_usart;
    gSerialFrame.RxIRQHandler(usart);
}

//------------------------------------------------------------------------------
void rx_init()
{
    Serial_RX_SetRxIRQHandler(RxIRQHandler);
    Serial_RX_Init();
}

//------------------------------------------------------------------------------
void checkrx()
{
    gSerialFrame.Update();
}

#endif  // #if defined(RX_ELRS)

