/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

// This file is copied with modifications from project Deviation,
// see http://deviationtx.com

/*
 * Based on sources from Betaflight 4.1 maintenance branch, git bc9715e
 * Modified for use in SilverLite FC
 */
#include "_my_config.h"
#if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)

extern "C" {
	#include "rx_spi.h"
}

#include "jee.h"
#include "drv_time.h"

#if defined(MATEKF411RX)
//------------------------------------------------------------------------------
// Instantiate the Pin<> objects needed for our software SPI implemenation
static Pin<'A', 15>		TRX_CS_PIN;
static Pin<'B', 3>		SPI_SCK;
static Pin<'B', 4>	    SPI_MISO;
static Pin<'B', 5>	    SPI_MOSI;
#elif defined(CRAZYBEEF3FS)
//------------------------------------------------------------------------------
// Instantiate the Pin<> objects needed for our software SPI implementation
// Examining Betaflight target.h you'll find this:
//#define RX_SPI_INSTANCE         SPI2
//#define RX_NSS_PIN              SPI2_NSS_PIN
// And this:
//#define USE_SPI_DEVICE_2 
//#define SPI2_NSS_PIN            PB12
//#define SPI2_SCK_PIN            PB13
//#define SPI2_MISO_PIN           PB14
//#define SPI2_MOSI_PIN           PB15

static Pin<'B', 12>		TRX_CS_PIN;
static Pin<'B', 13>		SPI_SCK;
static Pin<'B', 14>		SPI_MISO;
static Pin<'B', 15>		SPI_MOSI;
#else
	#error "Unsupported target"
#endif

//------------------------------------------------------------------------------
#define MOSIHIGH 	SPI_MOSI.write(1);
#define MOSILOW 	SPI_MOSI.write(0);
#define SCKHIGH 	SPI_SCK.write(1);
#define SCKLOW 		SPI_SCK.write(0);

#define READMISO 	SPI_MISO.read()
#define CS_ENABLE	TRX_CS_PIN.write(0);	// SPI chip select is active low
#define CS_DISABLE	TRX_CS_PIN.write(1);


//------------------------------------------------------------------------------
//
#define  __NOP()	__asm__ __volatile__("nop");

//------------------------------------------------------------------------------
//
// For CRAZYBEEF3FS, when this file IS added to SPEED_OPTIMISED_SRC:
#if defined(SPEED_OPTIMISED_SRC)
	// DELAY_SLOW, 2 nops, test_6_flyskyp reports 129us
	// DELAY_SLOW, 1 nops, test_6_flyskyp reports 114us
	// DELAY_SLOW, 0 nops, fails
	#define DELAY_SLOW	__NOP();

	// For CRAZYBEEF3FS, when this file is added to SPEED_OPTIMISED_SRC
	// DELAY_O3, 2 nops, test_6_flyskyp reports 121-125 us
	// DELAY_O3, 1 nop, test_6_flyskyp reports 110us
	// DELAY_O3, 0 nop, fails
	#define DELAY_O3	__NOP();

#else
	// For CRAZYBEEF3FS, when this file is NOT added to SPEED_OPTIMISED_SRC then
	// zero nops are needed but test_6_flyskyp reports 140us
	#define DELAY_SLOW
	#define	DELAY_O3
#endif



//------------------------------------------------------------------------------
extern "C" void rxSpiInit()
{
    TRX_CS_PIN.mode(Pinmode::out);
    TRX_CS_PIN.write(1);

	SPI_SCK.mode(Pinmode::out);
	SPI_MISO.mode(Pinmode::in_float);
	SPI_MOSI.mode(Pinmode::out);

	delay_us(1000);
}

//------------------------------------------------------------------------------
static void trx_spi_write( int data )
{
//#define __USE__ORIGINAL__	This is slightly slower
#if defined(__USE__ORIGINAL__)	
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
#else
	for ( int i =7; i >= 0; --i ) {
		SCKLOW
		if ( ( data >> i ) & 1 ) {
			MOSIHIGH
		} else {
			MOSILOW
		}
		SCKHIGH
		DELAY_O3
	}
	SCKLOW
	MOSILOW
#endif	
}

//------------------------------------------------------------------------------
static int trx_spi_read()
{
#if defined(__USE__ORIGINAL__)	
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
#else
	int recv = 0;
	MOSILOW
	for ( int i = 7; i >= 0; --i ) {
		recv = recv << 1;
		SCKHIGH
		DELAY_O3
		if ( READMISO ) {
			recv = recv | 1;
		}
		SCKLOW
		DELAY_O3
	}
	return recv;
#endif	
}

void rxSpiWriteByte(uint8_t data)
{
	CS_ENABLE
	trx_spi_write(data);
	CS_DISABLE
}

void rxSpiWriteCommand(uint8_t command, uint8_t data)
{
	CS_ENABLE
	trx_spi_write(command);
	trx_spi_write(data);
	CS_DISABLE
}

void rxSpiWriteCommandMulti(uint8_t command, const uint8_t *data, uint8_t length)
{
	CS_ENABLE
	trx_spi_write(command);
    while (length--)
    {
	    trx_spi_write(*data++);
    }
	CS_DISABLE
}

uint8_t rxSpiReadCommand(uint8_t command, uint8_t data)
{
    (void)(data);	// should always be 0xFF which means just do a pure read
	CS_ENABLE
	trx_spi_write(command);
	uint8_t result = trx_spi_read();
	CS_DISABLE
    return result;
}

void rxSpiReadCommandMulti(uint8_t command, uint8_t commandData, uint8_t *retData, uint8_t length)
{
    (void)(commandData);		// should always be 0xFF which means just do a pure read
	CS_ENABLE
	trx_spi_write(command);
    while (length--)
    {
        *retData++ = trx_spi_read();
    }
	CS_DISABLE
}

#endif  // #if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)
