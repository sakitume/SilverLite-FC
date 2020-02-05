//------------------------------------------------------------------------------
// Using max7456.c/.h from this repo for reference:
// https://github.com/jihlein/AQ32Plus

// See: https://www.sparkfun.com/datasheets/BreakoutBoards/MAX7456.pdf

// Some font stuff: 
// https://github.com/betaflight/betaflight-configurator/issues/1380
// https://github.com/betaflight/betaflight-configurator/pull/1392

#include <string.h>
#include <stdint.h>
#include "drv_osd.h"
#include "jee.h"

//------------------------------------------------------------------------------
// Instantiate the Pin<> objects needed for software SPI with the MAX7456
#include "drv_osd_spi_config.h"

//------------------------------------------------------------------------------
#define VIDEO_BUFFER_CHARS_PAL	480		// PAL is always more than NTSC
static uint8_t screenBuffer[VIDEO_BUFFER_CHARS_PAL];
static uint8_t shadowBuffer[VIDEO_BUFFER_CHARS_PAL];

#define MAX_CHARS2UPDATE    100
static uint8_t spiBuff[MAX_CHARS2UPDATE*6];

//------------------------------------------------------------------------------
extern void delay_ms(uint32_t ms);
extern void delay_us(uint32_t us);

#define MOSIHIGH 	SPI_MOSI.write(1);
#define MOSILOW 	SPI_MOSI.write(0);
#define SCKHIGH 	SPI_SCK.write(1);
#define SCKLOW 		SPI_SCK.write(0);

#define READMISO 	SPI_MISO.read()
#define CS_ENABLE	OSD_CS_Pin.write(0);	// SPI chip select is active low
#define CS_DISABLE	OSD_CS_Pin.write(1);

#define  __NOP()	__asm__ __volatile__("nop");
#if defined(__NOP)
	// Need at least 3 nops with OMINIBUS4 board (STM32F405)
	#if defined(OMNIBUS)
	#define DELAY_SLOW __NOP(); __NOP(); __NOP(); // __NOP(); __NOP();
	#else
	#define DELAY_SLOW
	#endif
#else
	// Initializing 'count' only (no while loop) works unless you use -O3 optimization
	//#define DELAY_SLOW	count = 0;
	#define DELAY_SLOW count = 0; while ( count-- );
	volatile static uint32_t count;
#endif	

//------------------------------------------------------------------------------
static void SPI_WRITE( int data )
{
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
}

//------------------------------------------------------------------------------
static int SPI_READ()
{
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
}

//------------------------------------------------------------------------------
static uint8_t spi_read_register(uint8_t r)
{
	SPI_WRITE(r);
	return SPI_READ();
}

//------------------------------------------------------------------------------
static void spi_write_buffer(uint8_t *buff, uint32_t numBytes)
{
	while (numBytes--)
	{
		SPI_WRITE(*buff++);
	}
}

//------------------------------------------------------------------------------
static void spi_write_register(uint8_t r, uint8_t data)
{
	static uint8_t buff[2];
	buff[0] = r;
	buff[1] = data;
	spi_write_buffer(buff, 2);
}

///////////////////////////////////////////////////////////////////////////////
#define NTSC                0
#define PAL                 1
#define AUTO				2

//MAX7456 Registers
#define VM0_REG             0x00
#define VM1_REG             0x01
#define DMM_REG             0x04
#define DMAH_REG            0x05
#define DMAL_REG            0x06
#define DMDI_REG            0x07
#define CMM_REG             0x08
#define CMAH_REG            0x09
#define RB0_REG             0x10
#define CMAL_REG            0x0A
#define CMDI_REG            0x0B
#define STAT_REG            0xA0

#define READ_MAX7456_REG    0x80

//MAX7456 Commands
#define CLEAR_DISPLAY       0x04
#define CLEAR_DISPLAY_VERT  0x06
#define END_STRING          0xFF
#define WRITE_NVR           0xA0

#define WHITE_LEVEL_80      0x03
#define WHITE_LEVEL_90      0x02
#define WHITE_LEVEL_100     0x01
#define WHITE_LEVEL_120     0x00

#define MAX_FONT_ROM        0xFF
#define STATUS_REG_NVR_BUSY 0x20
#define NVM_RAM_SIZE        0x36

///////////////////////////////////////////////////////////////////////////////
// MAX7456 Variables
///////////////////////////////////////////////////////////////////////////////

static uint8_t  osdDisabled;
static uint16_t maxScreenSize;
static uint16_t maxScreenRows;
static uint8_t  enableDisplay;
static uint8_t  enableDisplayVert;
static uint8_t  max7456Reset;
static uint8_t  disableDisplay;

///////////////////////////////////////////////////////////////////////////////
// Detect Video Standard
///////////////////////////////////////////////////////////////////////////////

static void detectVideoStandard()
{
    uint8_t pal = 0;	// 0 = false, 1 = true, 2 = auto detect from video in signal

    // if autodetect enabled modify the default if signal is present on either standard
    // otherwise default is preserved

	if (pal == AUTO) 
	{
		CS_ENABLE
		uint8_t stat = spi_read_register(STAT_REG);
		CS_DISABLE

		if (stat & 0x01)
			pal = PAL;

		if (stat & 0x02)
			pal = NTSC;
			
		if ((pal != PAL) && (pal != NTSC))
			pal = PAL;
	}

    if (pal)
    {
        maxScreenSize     = 480;
        maxScreenRows     = 16;
        enableDisplay     = 0x48;
        enableDisplayVert = 0x4C;
        max7456Reset      = 0x42;
        disableDisplay    = 0x40;
    }
    else
    {
        maxScreenSize     = 390;
        maxScreenRows     = 13;
        enableDisplay     = 0x08;
        enableDisplayVert = 0x0C;
        max7456Reset      = 0x02;
        disableDisplay    = 0x00;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Initialize MAX7456
///////////////////////////////////////////////////////////////////////////////

static void initMax7456()
{
    detectVideoStandard();

    // Soft reset the MAX7456 - clear display memory
    CS_ENABLE
    spi_write_register(VM0_REG, max7456Reset);
    CS_DISABLE

    delay_ms(1);

    // Set white level to 90% for all rows
    CS_ENABLE
    for (unsigned i = 0; i < maxScreenRows; i++ )
	{
//        spi_write_register(RB0_REG + i, WHITE_LEVEL_90 );
        spi_write_register(RB0_REG + i, WHITE_LEVEL_120 );
//        spi_write_register(RB0_REG + i, WHITE_LEVEL_80 );
	}
    spi_write_register(VM0_REG, enableDisplay);	//ensure device is enabled
    CS_DISABLE

	// 
	osd_clear();
}

//////////////////////////////////////////////////////////////////////////////
// Reset MAX7456
///////////////////////////////////////////////////////////////////////////////

static void resetMax7456()
{
    // force soft reset on Max7456
    CS_ENABLE
    spi_write_register(VM0_REG, max7456Reset);
    CS_DISABLE

    delay_ms(500);

    // set all rows to same character white level, 90%
    CS_ENABLE
    for (unsigned x = 0; x < maxScreenRows; x++)
	{
        spi_write_register(RB0_REG + x, WHITE_LEVEL_90);
	}
    spi_write_register(VM0_REG, enableDisplay);
    delay_ms(100);
    CS_DISABLE
}

///////////////////////////////////////////////////////////////////////////////
// Show MAX7456 Font
///////////////////////////////////////////////////////////////////////////////

//show all chars on 24 wide grid
static void showMax7456Font(void)
{
    // clear the screen
    CS_ENABLE
    spi_write_register(DMM_REG, CLEAR_DISPLAY);
    CS_DISABLE

    delay_us(50); // clearing display takes 20uS so wait some...

    // show all characters on screen (actually 0-254)
    CS_ENABLE
    spi_write_register(DMM_REG,  0x01);  // 16 bit trans w/o background, autoincrement
    spi_write_register(DMAH_REG,    0);  // set start address high
    spi_write_register(DMAL_REG,   33);  // set start address low (line 1 col 3 (0 based)
    for (unsigned x = 0; x < 255; x++)
    {
        spi_write_register(DMDI_REG, x);
        if ((x%24)==23)
        {
            for (unsigned i = 0; i < 6; i++)
			{
                spi_write_register(DMDI_REG, 0);
			}
		}
    }
    spi_write_register(DMDI_REG, END_STRING);
    CS_DISABLE
}


///////////////////////////////////////////////////////////////////////////////
// Wait for NVM
///////////////////////////////////////////////////////////////////////////////
static void waitNVM()
{
    while (spi_read_register(STAT_REG) & STATUS_REG_NVR_BUSY)
	{
		// wait
	}
}

///////////////////////////////////////////////////////////////////////////////
// Write NVM Character
///////////////////////////////////////////////////////////////////////////////
static void writeNVMcharacter(uint8_t ch, const unsigned index, uint8_t* fontData)
{
    CS_ENABLE
	{
		spi_write_register(VM0_REG, disableDisplay);
		spi_write_register(CMAH_REG, ch);  // set start address high
		for (unsigned x = 0; x < NVM_RAM_SIZE; x++) // write out 54 (out of 64) bytes of character to shadow ram
		{
			spi_write_register(CMAL_REG, x); // set start address low
			spi_write_register(CMDI_REG, fontData[index+x]);
		}
		// transfer a 54 bytes from shadow ram to NVM
		spi_write_register(CMM_REG, WRITE_NVR);
		waitNVM(); // NVM should be busy around 12ms
		spi_write_register(VM0_REG, enableDisplayVert);
	}
    CS_DISABLE
}

///////////////////////////////////////////////////////////////////////////////
// Download MAX7456 Font Data
///////////////////////////////////////////////////////////////////////////////

void downloadMax7456Font(uint8_t* fontData)
{
    for (unsigned ch = 0; ch < 256; ch++)
    {
        writeNVMcharacter(ch, 64*ch, fontData);
        delay_ms(30);	// TODO, XXX. Is this needed since waitNVM() is used 
    }

    // force soft reset on Max7456
    resetMax7456();
    showMax7456Font();
}

//------------------------------------------------------------------------------
void osd_hide(void)
{
	if (!osdDisabled)
	{
		osdDisabled = 1;
		CS_ENABLE
		spi_write_register(VM0_REG, disableDisplay);
		CS_DISABLE
	}
}

//------------------------------------------------------------------------------
void osd_show(void)
{
	if (osdDisabled)
	{
		osdDisabled = 0;
		CS_ENABLE
		spi_write_register(VM0_REG, enableDisplay);
		CS_DISABLE
	}
}

//------------------------------------------------------------------------------
// Walks thru the screenBuffer[] comparing against the shadowBuffer[], building
// up a command buffer for any changed character cells. If command buffer
// reaches a certain size we'll stop traversing the screen buffers and remember
// where we stopped, restarting from that point on the next call to osd_refresh()
//
void osd_refresh(void)
{
	static unsigned lastPos = 0;

	// Need 3 to 4 register writes for a single character
	// Need 4 to 5 register writes for a single character with auto-increment mode
	int buffMax = sizeof(spiBuff) - (2*5);	

	int 		autoIncrementMode = -1;	// -1 means unknown, so set it. 0 means disabled, 1 means enabled
	unsigned 	pos = lastPos;
	int 		buff_len = 0;
	while (buff_len < buffMax)
	{
		// Advance while shadow buffer and screen buffer match
		while ((pos < maxScreenSize) && (shadowBuffer[pos] == screenBuffer[pos]))
		{
			pos++;
		}

		// See how long of a run (of diffs) there are
		//
		// Drawing a run takes 3 or 4 register writes to start and stop,
		// then only one register write for each character thereafter.
		//
		// Without auto-increment, each character requires 3 register writes
		unsigned run = pos;
		while ((run < maxScreenSize) && (shadowBuffer[run] != screenBuffer[run]))
		{
			run++;
		}

		//
		int runLen = run - pos;
		if (runLen <= 0)
		{
			// Reached end of screen and found nothing to update
			pos = 0;
			break;
		}

		if (runLen <= 1)
		{
			// if auto increment mode is set (or unknown) then disable it
			if (!(autoIncrementMode == 0))
			{
				autoIncrementMode = 0;
				spiBuff[buff_len++] = DMM_REG;
				spiBuff[buff_len++] = 0;
			}

			uint8_t ch = screenBuffer[pos];
			spiBuff[buff_len++] = DMAH_REG;		spiBuff[buff_len++] = pos >> 8;
			spiBuff[buff_len++] = DMAL_REG;		spiBuff[buff_len++] = pos & 0xff;
			spiBuff[buff_len++] = DMDI_REG;		spiBuff[buff_len++] = ch;
			shadowBuffer[pos++]	= ch;
		}
		else
		{
			// if auto increment mode is disabled (or unknown) then enable it
//XXX			if (autoIncrementMode <= 0)
			{
				autoIncrementMode = 1;
				spiBuff[buff_len++] = DMM_REG;
				spiBuff[buff_len++] = 1;
			}
			spiBuff[buff_len++] = DMAH_REG;		spiBuff[buff_len++] = pos >> 8;
			spiBuff[buff_len++] = DMAL_REG;		spiBuff[buff_len++] = pos & 0xff;

			// Clamp the number of chars to output if needed so that we
			// don't overrun our spiBuff[]
			unsigned buffAvail 	= buffMax - buff_len - (1*2);	// 1 reg write (2bytes) to terminate
			unsigned maxChars	= buffAvail >> 1;	// div by 2 to get max chars we can write
			if ((unsigned)runLen > maxChars)
			{
				runLen = maxChars;
			}
			while (runLen--)
			{
				uint8_t ch = screenBuffer[pos];
				spiBuff[buff_len++] = DMDI_REG;		spiBuff[buff_len++] = ch;
				shadowBuffer[pos++]	= ch;
			}

			// Terminate auto increment mode
			spiBuff[buff_len++] = DMDI_REG;		spiBuff[buff_len++] = 0xff;
		}

		CS_ENABLE
		spi_write_buffer(spiBuff, buff_len);
		CS_DISABLE

//XXX		buff_len = 0;
	}

	lastPos = pos;
}

//------------------------------------------------------------------------------
static void osd_write(const char* str, int len, uint8_t y, uint8_t x)
{
	// If num characters to write will be zero then exit
	int strLen;
	if (str)
	{
		strLen = strlen(str);
	}
	else
	{
		strLen = 0;
	}
	if (len < 0)
	{
		len = strLen;
	}
	if (!len && !strLen)
	{
		return;
	}

   	uint16_t offset = y * 30 + x;
	for (int i = 0; i < len; i++)
	{
		screenBuffer[offset++] = (i < strLen) ? str[i] : ' ';
	}
}

//------------------------------------------------------------------------------
void osd_init()
{
	OSD_CS_Pin.mode(Pinmode::out);
	SPI_SCK.mode(Pinmode::out);
	SPI_MISO.mode(Pinmode::in_float);
	SPI_MOSI.mode(Pinmode::out);

	CS_DISABLE
//	delay_ms(50);	// 33 shows nothing, 35 works, so let's use 50
	delay_ms(150);	// ..but 150 should provide an even safer margin

	initMax7456();

//	showMax7456Font();

	osd_show();
}

//------------------------------------------------------------------------------
void osd_print(int row, int col, const char *s)
{
	osd_write(s, -1, row, col);
}

//------------------------------------------------------------------------------
void osd_erase(int row, int col, int count)
{
	osd_write(nullptr, count, row, col);
}

//------------------------------------------------------------------------------
void osd_clear()
{
	memset(screenBuffer, ' ', maxScreenSize);
}

//------------------------------------------------------------------------------
uint8_t osd_get_max_rows()
{
	return maxScreenRows;
}