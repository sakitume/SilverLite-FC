#include "trx.h"    // defines TRX_LT8900 (or not)

#if defined(TRX_LT8900)
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
extern void trx_spi_write(int data);
extern int trx_spi_read();
extern void trx_spi_init();
extern void trx_spi_cs_enable();
extern void trx_spi_cs_disable();

extern "C" {
    extern void failloop( int );
}


//------------------------------------------------------------------------------
#define REGISTER_READ       0b10000000  //bin
#define REGISTER_WRITE      0b00000000  //bin
#define REGISTER_MASK       0b01111111  //bin

#define R_CHANNEL           7
#define CHANNEL_RX_BIT      7
#define CHANNEL_TX_BIT      8
#define CHANNEL_MASK        0b01111111  ///bin
#define DEFAULT_CHANNEL     0x30

#define R_CURRENT           9
#define CURRENT_POWER_SHIFT 12
#define CURRENT_POWER_MASK  0b1111000000000000
#define CURRENT_GAIN_SHIFT  7
#define CURRENT_GAIN_MASK   0b0000011110000000

/* LT8910S only */
#define R_DATARATE          44
#define DATARATE_MASK       0x00FF
#define DATARATE_1MBPS      0x0100
#define DATARATE_250KBPS    0x0400
#define DATARATE_125KBPS    0x0800
#define DATARATE_62KBPS     0x1000

#define R_SYNCWORD1         36
#define R_SYNCWORD2         37
#define R_SYNCWORD3         38
#define R_SYNCWORD4         39

#define R_PACKETCONFIG      41
#define PACKETCONFIG_CRC_ON             0x8000
#define PACKETCONFIG_SCRAMBLE_ON        0x4000
#define PACKETCONFIG_PACK_LEN_ENABLE    0x2000
#define PACKETCONFIG_FW_TERM_TX         0x1000
#define PACKETCONFIG_AUTO_ACK           0x0800
#define PACKETCONFIG_PKT_FIFO_POLARITY  0x0400

#define R_STATUS            48
#define STATUS_CRC_BIT      15
#define STATUS_PKT_FLAG_BIT 6


#define R_FIFO              50
#define R_FIFO_CONTROL      52

//------------------------------------------------------------------------------
static uint8_t _channel;
static bool telemetryPending;

//------------------------------------------------------------------------------
// Forward declarations
void trx_SetAddr(uint8_t addr[]);
void trx_SetChannel(uint8_t channel);
void trx_SetToRXMode();

//------------------------------------------------------------------------------
static void lt_writeRegister2(uint8_t reg, uint8_t high, uint8_t low)
{
    trx_spi_cs_enable();

    trx_spi_write(REGISTER_WRITE | (REGISTER_MASK & reg));
    trx_spi_write(high);
    trx_spi_write(low);

    trx_spi_cs_disable();
}

//------------------------------------------------------------------------------
static void lt_writeRegister(uint8_t reg, uint16_t data)
{
    uint8_t high = data >> 8;
    uint8_t low = data & 0xFF;
    lt_writeRegister2(reg, high, low);
}

//------------------------------------------------------------------------------
static uint16_t lt_readRegister(uint8_t reg)
{
    trx_spi_cs_enable();

    trx_spi_write(REGISTER_READ | (REGISTER_MASK & reg));
    uint8_t high = trx_spi_read();
    uint8_t low = trx_spi_read();

    trx_spi_cs_disable();

    return (high << 8 | low);
}

//------------------------------------------------------------------------------
static void lt_setCurrentControl(uint8_t power, uint8_t gain)
{
    lt_writeRegister(R_CURRENT,
        ((power << CURRENT_POWER_SHIFT) & CURRENT_POWER_MASK) |
        ((gain << CURRENT_GAIN_SHIFT) & CURRENT_GAIN_MASK));
}

//------------------------------------------------------------------------------
void trx_Init()
{
    trx_spi_init();

    lt_writeRegister(0, 0x6fe0);
    lt_writeRegister(1, 0x5681);
    lt_writeRegister(2, 0x6617);
    lt_writeRegister(4, 0x9cc9); //why does this differ from powerup (5447)
    lt_writeRegister(5, 0x6637); //why does this differ from powerup (f000)
    lt_writeRegister(8, 0x6c90); //power (default 71af) UNDOCUMENTED

    lt_setCurrentControl(4, 0); // power & gain.

    lt_writeRegister(10, 0x7ffd); //bit 0: XTAL OSC enable
    lt_writeRegister(11, 0x0000); //bit 8: Power down RSSI (0=  RSSI operates normal)
    lt_writeRegister(12, 0x0000);
    lt_writeRegister(13, 0x48bd); //(default 4855)

    lt_writeRegister(22, 0x00ff);
    lt_writeRegister(23, 0x8005); //bit 2: Calibrate VCO before each Rx/Tx enable
    lt_writeRegister(24, 0x0067);
    lt_writeRegister(25, 0x1659);
    lt_writeRegister(26, 0x19e0);
    lt_writeRegister(27, 0x1300); //bits 5:0, Crystal Frequency adjust
    lt_writeRegister(28, 0x1800);

/*
Preamble length is 4 bytes
SyncWord (RX_TX address) is 32bits (4 bytes) long and initially set to: 0x00000000
Trailer length is 8bits
*/
    //fedcba9876543210
    lt_writeRegister(32, 0x6A00); //AAABBCCCDDEEFFFG  A preamble length, B, syncword length, c trailer length, d packet type
    //                  E FEC_type, F BRCLK_SEL, G reserved

    //         AAAB BCCC DDEE FFFG
    //0x6A00 = 0110 1010 0000 0000 = preamble 011 (4 bytes), B 01 (32 bits)

    //         AAA BB CCC DD EE FFF G
    //0x6A00 = 011 01 010 00 00 000 0 = A preamble length: 011 (4 bytes), B syncword length: 01 (32 bits), C trailer length: 010 (8 bits), D packet type: 00 (NRZ law data), E fec type: 00 (No FEC), F BRCLK_SEL: 000 (keep low)

    lt_writeRegister(33, 0x3fc7);
    lt_writeRegister(34, 0x2000); //
    lt_writeRegister(35, 0x0300); //POWER mode,  bit 8/9 on = retransmit = 3x (default)

//    setSyncWord(0x03805a5a03800380);
//    setSyncWord(0xAABBCCDDEEFF0011);

    lt_writeRegister(40, 0x4401); //max allowed error bits = 0 (01 = 0 error bits)
    lt_writeRegister(R_PACKETCONFIG,
                  PACKETCONFIG_CRC_ON |
                  PACKETCONFIG_PACK_LEN_ENABLE |
                  PACKETCONFIG_FW_TERM_TX);

    lt_writeRegister(42, 0xfdb0);
    lt_writeRegister(43, 0x000f);

/*
    //check the variant.
    _isLT8910 = setDataRate(LT8910_62KBPS);
    //return to defaults.
    setDataRate(LT8900_1MBPS);
*/    

    //15:8, 01: 1Mbps 04: 250Kbps 08: 125Kbps 10: 62.5Kbps

    lt_writeRegister(R_FIFO, 0x0000); //TXRX_FIFO_REG (FIFO queue)

    lt_writeRegister(R_FIFO_CONTROL, 0x8080); //Fifo Rx/Tx queue reset

    delay_ms(200);
    lt_writeRegister(R_CHANNEL, (1L << CHANNEL_TX_BIT)); //set TX mode.  (TX = bit 8, RX = bit 7, so RX would be 0x0080)
    delay_ms(2);
    lt_writeRegister(R_CHANNEL, _channel); // Frequency = 2402 + channel

#define RADIO_CHECK
#ifdef RADIO_CHECK
    int rxcheck = lt_readRegister(10); 	// Datasheet and testing shows power-up reset value as 0x7ffd
    // should be 0x7ffd
    if ( rxcheck != 0x7ffd ) {
        failloop( 3 );
    }
#endif

    //
    lt_setCurrentControl(15, 15);
/*
Preamble length is 4 bytes
Trailer length is 8bits
CRC is enabled
PACKET LENGTH prefix byte is enabled
1Mbps data rate
SyncWord (RX_TX address) is 4 bytes long (32bits) and initially set to: 0x00000000
Initial channel is set to 0
*/
    //fedcba9876543210
    lt_writeRegister(32, 0x6A00); //AAABBCCCDDEEFFFG  A preamble length, B, syncword length, c trailer length, d packet type
    //                  E FEC_type, F BRCLK_SEL, G reserved

    //         AAAB BCCC DDEE FFFG
    //0x6A00 = 0110 1010 0000 0000 = preamble 011 (4 bytes), B 01 (32 bits)

    //         AAA BB CCC DD EE FFF G
    //0x6A00 = 011 01 010 00 00 000 0 = A preamble length: 011 (4 bytes), B syncword length: 01 (32 bits), C trailer length: 010 (8 bits), D packet type: 00 (NRZ law data), E fec type: 00 (No FEC), F BRCLK_SEL: 000 (keep low)


    // Multiprotocol emulation of LT8900 works with CRC
    // and I was able to do that using 0xAA for the CRC_INITIAL_DATA 
    uint8_t crc_initial_data = 0xAA;
    lt_writeRegister(R_PACKETCONFIG,
        PACKETCONFIG_CRC_ON |
        PACKETCONFIG_PACK_LEN_ENABLE |  // The first word placed into FIFO will be the packet length
        PACKETCONFIG_FW_TERM_TX |       // End TX when packet length is reached
        crc_initial_data
    );
    lt_writeRegister(R_DATARATE, DATARATE_1MBPS);

    trx_SetAddr(nullptr);
    trx_SetChannel(0);
    trx_SetToRXMode();

#if 0
    for (int i = 0; i <= 50; i++)
    {
        uint16_t value = lt_readRegister(i);
        XPRINTF("%d = %04x\r\n", i, value);
    }
#endif
}

//------------------------------------------------------------------------------
void trx_SetAddr(uint8_t addr[])
{
    if (!addr)
    {
        static uint8_t bindSyncWord[4] = { 0x12, 0x34, 0x56, 0x78 };
        // Using <0,0,0,0> results in 100% CRC errors
//        static uint8_t bindSyncWord[4] = { 0, 0, 0, 0 };
        addr = bindSyncWord;
    }

    #define R_SYNCWORD1         36
    #define R_SYNCWORD2         37
    #define R_SYNCWORD3         38
    #define R_SYNCWORD4         39

    // For 32bit syncword: first 2 bytes go to R_SYNCWORD4, next 2 bytes go to R_SYNCWORD1
    // R_SYNCWORD2 and R_SYNCWORD3 are ignored

    uint16_t data;
    data = (uint16_t)addr[0] << 8 | addr[1];
    lt_writeRegister(R_SYNCWORD4, data);
    data = (uint16_t)addr[2] << 8 | addr[3];
    lt_writeRegister(R_SYNCWORD1, data);
}

//------------------------------------------------------------------------------
// This is called by nextchannel() when we frequency hop. Due to how LT8900
// works we can't just set the CHANNEL field as the RX state machine
// is already running. Instead we must restart RX mode (set RX_EN bit)
// while also setting the CHANNEL field. Turning on RX mode is
// okay because trx_SetChannel()/nextchannel() are always used in the
// context of hopping to the next channel for the next receive stage
// 
void trx_SetChannel(uint8_t channel)
{
    telemetryPending = false;
    _channel = channel & CHANNEL_MASK;
// Not necessary to stop RX first   lt_writeRegister(R_CHANNEL, 0);
    lt_writeRegister(R_FIFO_CONTROL, 0x0080);   //flush rx
    lt_writeRegister(R_CHANNEL, _channel | (1L << CHANNEL_RX_BIT)); //enable RX
}

//------------------------------------------------------------------------------
bool trx_TelemetrySent()
{
    bool pktFlagSet = (lt_readRegister(R_STATUS) & (1 << 6)) != 0;
    if (pktFlagSet)
    {
        telemetryPending = false;
    }
    return pktFlagSet;
}

//------------------------------------------------------------------------------
void trx_FlushTX()
{
    telemetryPending = false;
    lt_writeRegister(R_FIFO_CONTROL, 0x8000);	// flush tx
}

//------------------------------------------------------------------------------
// Used by checkrx() when packet hasn't been received within the packet_period
// The RX fifo flush (setting CLR_R_PTR of register 52) should be sufficient
// but if PKT_FLAG is set I'll do some additional "cleanup"
void trx_FlushRX()
{
    lt_writeRegister(R_CHANNEL, _channel);      // turn off rx/tx
    lt_writeRegister(R_FIFO_CONTROL, 0x0080);   // flush rx
    lt_writeRegister(R_CHANNEL, _channel | (1L << CHANNEL_RX_BIT)); // re-enable RX
}

//------------------------------------------------------------------------------
void trx_SetToRXMode()
{
    telemetryPending = false;
    lt_writeRegister(R_CHANNEL, _channel);      //turn off rx/tx
    lt_writeRegister(R_FIFO_CONTROL, 0x0080);   //flush rx
    lt_writeRegister(R_CHANNEL, _channel | (1L << CHANNEL_RX_BIT)); //enable RX
}

//------------------------------------------------------------------------------
void trx_PrepareToSendTelemetry()
{
    lt_writeRegister(R_CHANNEL, 0x0000);        // disable RX and TX
    lt_writeRegister(R_FIFO_CONTROL, 0x8000);   // flush tx FIFO
}

//------------------------------------------------------------------------------
void trx_SendTelemetryPacket(int data[], int size)
{
    // Packets are sent in 16bit words, and the first word will be the packet size.
    // Start spitting out words until we are done.
    uint8_t pos = 0;
    lt_writeRegister2(R_FIFO, size, data[pos++]);
    while (pos < size)
    {
        uint8_t msb = data[pos++];
        uint8_t lsb = data[pos++];

        lt_writeRegister2(R_FIFO, msb, lsb);
    }

    // Now we can enable TX
    lt_writeRegister(R_CHANNEL, _channel | (1L << CHANNEL_TX_BIT));

    telemetryPending = true;
}

//------------------------------------------------------------------------------
uint8_t trx_PacketAvailable()
{
    // If telemetry packet was sent and we're still waiting for it to complete
    // then RX isn't active and therefore no packet could be available.
    // The PKT_FLAG *might* be set, but it would mean the telemetry packet
    // has finished sending, *not* that a RX packet was received. So check
    // for that and early exit with failure to avoid a false positive on RX
    // packet being available.
    if (telemetryPending)
    {
        return false;
    }

    // Check if PKT_FLAG is set
    int status = lt_readRegister(R_STATUS);
    if (status & (1 << 6))
    {
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool trx_ReadPayload(int buffer[], int size)
{
    int status = lt_readRegister(R_STATUS);
    if ( 0 == (status & (1 << 6)))
    {
        return false;
    }

    // Note: Per the documentation, reading from FIFO is supposed to clear the PKT_FLAG in STATUS register.
    // But I've observed that this isn't true. Maybe I'm supposed to read all of the
    // bytes from the FIFO

    // Note: readRegister() pulls 2 bytes at a time with first byte
    // being in upper 8bits and second byte in lower 8 bits of return value
    uint16_t data = lt_readRegister(R_FIFO);

    bool error = false;
    if (status & (1L << STATUS_CRC_BIT))
    {
        extern int crc_errors;
        crc_errors++;
        error = true;
    }
    else
    {
        uint8_t payloadSize = data >> 8;        // >> 8 to access 1st byte retrieved from FIFO
        if (payloadSize != size)
        {
            error = true;
        }
        else
        {
            uint8_t payloadFirstByte = data & 0xFF; // & 0xFF to access 2nd byte retrieved from FIFO
            uint8_t pos = 0;
            buffer[pos++] = payloadFirstByte;
            while (pos < payloadSize)
            {
                data = lt_readRegister(R_FIFO);
                buffer[pos++] = data >> 8;
                if (pos < payloadSize)
                {
                    buffer[pos++] = data & 0xFF;
                }
            }
        }
    }

    // Need to ensure PKT_FLAG is cleared, an extra step is necessary
    // if an error occurred (maybe because I didn't read all of the bytes
    // that were written into the FIFO?). The extra step is to clear the RX_EN bit
    if (error)
    {
        lt_writeRegister(R_CHANNEL, 0); // disable RX
    }

    // Re-enable RX, this will also clear the PKT_FLAG in R_STATUS register
    lt_writeRegister(R_FIFO_CONTROL, 0x0080);                                     //flush rx
    lt_writeRegister(R_CHANNEL, _channel | (1L << CHANNEL_RX_BIT)); //enable RX, it will become fully active 2us (page 36 timing diagram) to 10us (page 37 flowchart) later

#if 0   // I've confirmed PKT_FLAG is correctly cleared
    if (error) 
    {
        uint16_t value = lt_readRegister(R_STATUS);
        if (bitRead(value, 6) != 0)
        {
            Serial.println("ERROR: PKT_FLAG not reset");
        }
    }
#endif    
     
    return !error;
}

#endif  // #if defined(TRX_LT8900)
