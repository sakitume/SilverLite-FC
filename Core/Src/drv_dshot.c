// Enable this for 3D. The 'Motor Direction' setting in BLHeliSuite must
// be set to 'Bidirectional' (or 'Bidirectional Rev.') accordingly:
#define BIDIRECTIONAL

// IDLE_OFFSET is added to the throttle. Adjust its value so that the motors
// still spin at minimum throttle.
#define IDLE_OFFSET 30 // 4S


#include <stdbool.h>

#include "defines.h"
#include "drv_dshot.h"
#include "drv_time.h"
#include "hardware.h"
#include "main.h"

#ifdef DSHOT_DRIVER

extern int onground;

int pwmdir = 0;
static uint32_t motor_data[ 48 ] = { 0 }; // Access to uint32_t array is overall 4 us faster than uint8_t.

static void make_packet( uint8_t number, uint16_t value, bool telemetry );
static void bitbang_data( void );

void pwm_init()
{
	for ( int i = 0; i <= 3; ++i ) {
		pwm_set( i, 0 );
	}
}

int idle_offset = IDLE_OFFSET; // gets corrected by battery_scale_factor in battery.c
void pwm_set( uint8_t number, float pwm )
{
	if ( pwm < 0.0f ) {
		pwm = 0.0f;
	}
	if ( pwm > 0.999f ) {
		pwm = 0.999f;
	}

	uint16_t value = 0;

#ifdef BIDIRECTIONAL

	if ( pwmdir == FORWARD ) {
		// maps 0.0 .. 0.999 to 48 + IDLE_OFFSET .. 1047
		value = 48 + idle_offset + (uint16_t)( pwm * ( 1000 - idle_offset ) );
	} else if ( pwmdir == REVERSE ) {
		// maps 0.0 .. 0.999 to 1048 + IDLE_OFFSET .. 2047
		value = 1048 + idle_offset + (uint16_t)( pwm * ( 1000 - idle_offset ) );
	}

#else

	// maps 0.0 .. 0.999 to 48 + IDLE_OFFSET * 2 .. 2047
	value = 48 + idle_offset * 2 + (uint16_t)( pwm * ( 2001 - idle_offset * 2 ) );

#endif

	if ( onground ) {
		value = 0; // stop the motors
	}

	make_packet( number, value, false );

	if ( number == 3 ) {
		__disable_irq();
		bitbang_data();
		__disable_irq();
	}
}

static void make_packet( uint8_t number, uint16_t value, bool telemetry )
{
	uint16_t packet = ( value << 1 ) | ( telemetry ? 1 : 0 ); // Here goes telemetry bit
	// compute checksum
	uint16_t csum = 0;
	uint16_t csum_data = packet;
	for ( uint8_t i = 0; i < 3; ++i ) {
		csum ^= csum_data; // xor data by nibbles
		csum_data >>= 4;
	}
	csum &= 0xf;
	// append checksum
	packet = ( packet << 4 ) | csum;

	// generate pulses for whole packet
	for ( uint8_t i = 0; i < 16; ++i ) {
		if ( packet & 0x8000 ) { // MSB first
			motor_data[ i * 3 + 0 ] |= 1 << number;
			motor_data[ i * 3 + 1 ] |= 1 << number;
			motor_data[ i * 3 + 2 ] |= 0 << number;
		} else {
			motor_data[ i * 3 + 0 ] |= 1 << number;
			motor_data[ i * 3 + 1 ] |= 0 << number;
			motor_data[ i * 3 + 2 ] |= 0 << number;
		}
		packet <<= 1;
	}
}

// Do not change anything between #pragma push and #pragma pop
// without redoing thorough timing measurements.
#ifndef __GNUC__
#pragma push
#pragma O2
#endif

static void bitbang_data()
{
	for ( uint8_t i = 0; i < 48; ++i ) {
		const uint32_t data = motor_data[ i ];
		motor_data[ i ] = 0;

		if ( data & 0x01 ) {
			gpioset( ESC1_GPIO_Port, ESC1_Pin );
		} else {
			_NOP_
			gpioreset( ESC1_GPIO_Port, ESC1_Pin );
		}

		if ( data & 0x02 ) {
			gpioset( ESC2_GPIO_Port, ESC2_Pin );
		} else {
			_NOP_
			gpioreset( ESC2_GPIO_Port, ESC2_Pin );
		}

		if ( data & 0x04 ) {
			gpioset( ESC3_GPIO_Port, ESC3_Pin );
		} else {
			_NOP_
			gpioreset( ESC3_GPIO_Port, ESC3_Pin );
		}

		if ( data & 0x08 ) {
			gpioset( ESC4_GPIO_Port, ESC4_Pin );
		} else {
			_NOP_
			gpioreset( ESC4_GPIO_Port, ESC4_Pin );
		}

		// Note: delay timings for -Os

		volatile static uint32_t count;
#if defined(STM32F405xx)
	#if 1
		// Dshot600, BLHeli_S BB2 (not supported by BB1)
		count = 6; while ( count-- ); // 5 to 7 is recognized as Dshot600
	#else
		// Dshot300, works on BB1
		count = 18; while ( count-- ); // 14 to 23 is recognized as Dshot300
	#endif
#elif defined(STM32F411xE)
	#if 1
		// Dshot600, BLHeli_S BB2 (not supported by BB1)
		_NOP_ _NOP_ _NOP_ _NOP_ _NOP_
		_NOP_ _NOP_ _NOP_ _NOP_ _NOP_
		_NOP_ _NOP_ _NOP_ _NOP_ _NOP_
		_NOP_ _NOP_ _NOP_ _NOP_ _NOP_
	#else
		// Dshot300, works on BB1
		count = 7; while ( count-- ); // 6 to 9 is recognized as Dshot300
	#endif
#else
	#error "Unknown MCU"
#endif
	}
}

#ifndef __GNUC__
#pragma pop
#endif

#define DSHOT_CMD_BEEP1 1
#define DSHOT_CMD_BEEP2 2
#define DSHOT_CMD_BEEP3 3
#define DSHOT_CMD_BEEP4 4
#define DSHOT_CMD_BEEP5 5 // 5 currently uses the same tone as 4 in BLHeli_S.

void motorbeep( bool motors_failsafe, int channel )
{
	static unsigned long motor_beep_time = 0;
	unsigned long time = gettime();
	extern char aux[];
	if ( ( motors_failsafe && time > 60e6f ) || aux[ channel ] ) {
		if ( motor_beep_time == 0 ) {
			motor_beep_time = time;
		}
		const unsigned long delta_time = time - motor_beep_time;
		uint8_t beep_command = 0;
		#define INTERVAL 250000
		if ( delta_time % 2000000 < INTERVAL ) {
			beep_command = DSHOT_CMD_BEEP1;
		} else if ( delta_time % 2000000 < INTERVAL * 2 ) {
			beep_command = DSHOT_CMD_BEEP3;
		} else if ( delta_time % 2000000 < INTERVAL * 3 ) {
			beep_command = DSHOT_CMD_BEEP2;
		} else if ( delta_time % 2000000 < INTERVAL * 4 ) {
			beep_command = DSHOT_CMD_BEEP4;
		}
		if ( beep_command != 0 ) {
			make_packet( 0, beep_command, true );
			make_packet( 1, beep_command, true );
			make_packet( 2, beep_command, true );
			make_packet( 3, beep_command, true );
			bitbang_data();
		}
	} else {
		motor_beep_time = 0;
	}
}

#if defined(TURTLE_MODE)
// DSHOT_CMD constants and basic algorithm (repeat 10 times, ensure telemetry bit set, etc)
// were determined from examining Betaflight source code
#define    DSHOT_CMD_SPIN_DIRECTION_NORMAL 		20
#define    DSHOT_CMD_SPIN_DIRECTION_REVERSED 	21

#define DSHOT_INITIAL_DELAY_US 10000
#define DSHOT_COMMAND_DELAY_US 1000

void pwm_set_direction(bool bNormalDirection)
{
	uint8_t command = bNormalDirection ? DSHOT_CMD_SPIN_DIRECTION_NORMAL : DSHOT_CMD_SPIN_DIRECTION_REVERSED;

	delay(DSHOT_INITIAL_DELAY_US - DSHOT_COMMAND_DELAY_US);
	int repeats = 10;
	while (repeats--)
	{
		delay(DSHOT_COMMAND_DELAY_US);

		make_packet( 0, command, true );
		make_packet( 1, command, true );
		make_packet( 2, command, true );
		make_packet( 3, command, true );
		bitbang_data();
	}
	delay(DSHOT_COMMAND_DELAY_US);
}
#endif

#endif // DSHOT_DRIVER
