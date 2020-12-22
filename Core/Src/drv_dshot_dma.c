// Original BWHOOP DMA code by JazzMac who based his work on http://www.cnblogs.com/shangdawei/p/4762035.html

// Enable this for 3D. The 'Motor Direction' setting in BLHeliSuite must be set to 'Bidirectional' (or 'Bidirectional Rev.') accordingly:
#define BIDIRECTIONAL

// IDLE_OFFSET is added to the throttle. Adjust its value so that the motors still spin at minimum throttle.
#define IDLE_OFFSET 30 // 4S

// Select Dshot1200, Dshot600, Dshot300, or Dshot150
// #define DSHOT 1200 // BLHeli_32 only
// #define DSHOT 600 // BLHeli_S BB2 (not supported by BB1)
#define DSHOT 300 // works on BB1
// #define DSHOT 150 // Dshot150 is too slow if the motors are on two different ports and 8k loop frequency is used.


#include <stdbool.h>

#include "defines.h"
#include "drv_dshot.h"
#include "drv_time.h"
#include "hardware.h"
#include "main.h"

#define DSHOT_BIT_TIME ( ( SYS_CLOCK_FREQ_MHZ * 1000 / DSHOT ) - 1 )
#define DSHOT_T0H_TIME ( DSHOT_BIT_TIME * 0.30 )
#define DSHOT_T1H_TIME ( DSHOT_BIT_TIME * 0.60 )

#ifdef DSHOT_DMA_DRIVER

extern int onground;

int pwmdir = FORWARD;

volatile int dshot_dma_phase = 0; // 1: port1st, 2: port2nd, 0: idle
uint16_t dshot_packet[ 4 ]; // 16bits dshot data for 4 motors

uint32_t motor_data_port1st[ 16 ] = { 0 }; // DMA buffer: reset output when bit data=0 at TOH timing
uint32_t motor_data_port2nd[ 16 ] = { 0 };

uint32_t dshot_port1st[ 1 ] = { 0 }; // sum of all motor pins at port1st
uint32_t dshot_port1st_off[ 16 ] = { 0 };
uint32_t dshot_port2nd[ 1 ] = { 0 }; // sum of all motor pins at port2nd
uint32_t dshot_port2nd_off[ 16 ] = { 0 };

GPIO_TypeDef * GPIO1st = 0;
GPIO_TypeDef * GPIO2nd = 0;

void pwm_init()
{
#if !defined(STM32F3)	// Already called by main() on F3 platform
	extern void MX_TIM1_Init( void );
	MX_TIM1_Init();
#endif	

	TIM1->ARR = DSHOT_BIT_TIME;
	TIM1->CCR1 = DSHOT_T0H_TIME;
	TIM1->CCR2 = DSHOT_T1H_TIME;

	// This driver supports Dshot pins being located on up to two distinct GPIO ports.
	GPIO1st = ESC1_GPIO_Port;
	if ( ESC2_GPIO_Port != GPIO1st ) {
		GPIO2nd = ESC2_GPIO_Port;
	}
	if ( ESC3_GPIO_Port != GPIO1st ) {
		GPIO2nd = ESC3_GPIO_Port;
	}
	if ( ESC4_GPIO_Port != GPIO1st ) {
		GPIO2nd = ESC4_GPIO_Port;
	}
	if ( ( ESC2_GPIO_Port != GPIO1st && ESC2_GPIO_Port != GPIO2nd ) ||
		( ESC3_GPIO_Port != GPIO1st && ESC3_GPIO_Port != GPIO2nd ) ||
		( ESC4_GPIO_Port != GPIO1st && ESC4_GPIO_Port != GPIO2nd ) )
	{
		extern void failloop( int );
		failloop( 7 );
	}

	if ( ESC1_GPIO_Port == GPIO1st ) {
		*dshot_port1st |= ESC1_Pin;
	} else {
		*dshot_port2nd |= ESC1_Pin;
	}
	if ( ESC2_GPIO_Port == GPIO1st ) {
		*dshot_port1st |= ESC2_Pin;
	} else {
		*dshot_port2nd |= ESC2_Pin;
	}
	if ( ESC3_GPIO_Port == GPIO1st ) {
		*dshot_port1st |= ESC3_Pin;
	} else {
		*dshot_port2nd |= ESC3_Pin;
	}
	if ( ESC4_GPIO_Port == GPIO1st ) {
		*dshot_port1st |= ESC4_Pin;
	} else {
		*dshot_port2nd |= ESC4_Pin;
	}

	// The bidir DMA driver needs to also inclrement the memory address associated with the
	// TIM1_CH2 DMA stream, so we also provide an array of 16 values here to be able to use
	// the same CubeMX initialization code.
	for ( uint8_t i = 0; i < 16; ++i ) {
		dshot_port1st_off[ i ] = ( *dshot_port1st ) << 16;
		dshot_port2nd_off[ i ] = ( *dshot_port2nd ) << 16;
	}
}

static void dshot_dma_port1st()
{
	extern TIM_HandleTypeDef htim1;
	extern DMA_HandleTypeDef hdma_tim1_up;
	extern DMA_HandleTypeDef hdma_tim1_ch1;
	extern DMA_HandleTypeDef hdma_tim1_ch2;

	HAL_TIM_Base_Stop( &htim1 );

	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_UPDATE );
	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_CC1 );
	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_CC2 );

	// This resets the hdma->State = HAL_DMA_STATE_READY; otherwise, no new transfer is started.
	HAL_DMA_Abort( &hdma_tim1_up );
	HAL_DMA_Abort( &hdma_tim1_ch1 );
	HAL_DMA_Abort( &hdma_tim1_ch2 );

	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_UPDATE );
	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_CC1 );
	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_CC2 );

	__HAL_DMA_ENABLE_IT( &hdma_tim1_ch2, DMA_IT_TC );

	HAL_DMA_Start( &hdma_tim1_up, (uint32_t)dshot_port1st, (uint32_t)&GPIO1st->BSRR, 16 );
	HAL_DMA_Start( &hdma_tim1_ch1, (uint32_t)motor_data_port1st, (uint32_t)&GPIO1st->BSRR, 16 );
	HAL_DMA_Start( &hdma_tim1_ch2, (uint32_t)dshot_port1st_off, (uint32_t)&GPIO1st->BSRR, 16 );

	__HAL_TIM_SET_COUNTER( &htim1, DSHOT_BIT_TIME );
	HAL_TIM_Base_Start( &htim1 );
}

static void dshot_dma_port2nd()
{
	extern TIM_HandleTypeDef htim1;
	extern DMA_HandleTypeDef hdma_tim1_up;
	extern DMA_HandleTypeDef hdma_tim1_ch1;
	extern DMA_HandleTypeDef hdma_tim1_ch2;

	HAL_TIM_Base_Stop( &htim1 );

	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_UPDATE );
	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_CC1 );
	__HAL_TIM_DISABLE_DMA( &htim1, TIM_DMA_CC2 );

	// This resets the hdma->State = HAL_DMA_STATE_READY; otherwise, no new transfer is started.
	HAL_DMA_Abort( &hdma_tim1_up );
	HAL_DMA_Abort( &hdma_tim1_ch1 );
	HAL_DMA_Abort( &hdma_tim1_ch2 );

	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_UPDATE );
	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_CC1 );
	__HAL_TIM_ENABLE_DMA( &htim1, TIM_DMA_CC2 );

	__HAL_DMA_ENABLE_IT( &hdma_tim1_ch2, DMA_IT_TC );

	HAL_DMA_Start( &hdma_tim1_up, (uint32_t)dshot_port2nd, (uint32_t)&GPIO2nd->BSRR, 16 );
	HAL_DMA_Start( &hdma_tim1_ch1, (uint32_t)motor_data_port2nd, (uint32_t)&GPIO2nd->BSRR, 16 );
	HAL_DMA_Start( &hdma_tim1_ch2, (uint32_t)dshot_port2nd_off, (uint32_t)&GPIO2nd->BSRR, 16 );

	__HAL_TIM_SET_COUNTER( &htim1, DSHOT_BIT_TIME );
	HAL_TIM_Base_Start( &htim1 );
}

// make dshot packet
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
	dshot_packet[ number ] = ( packet << 4 ) | csum;
}

// make dshot dma packet, then fire
static void dshot_dma_start()
{
	// generate dshot dma packet
	for ( uint8_t i = 0; i < 16; ++i ) {
		motor_data_port1st[ i ] = 0;
		motor_data_port2nd[ i ] = 0;

		if ( ! ( dshot_packet[ 0 ] & 0x8000 ) ) {
			if ( ESC1_GPIO_Port == GPIO1st ) {
				motor_data_port1st[ i ] |= ESC1_Pin << 16;
			} else {
				motor_data_port2nd[ i ] |= ESC1_Pin << 16;
			}
		}
		if ( ! ( dshot_packet[ 1 ] & 0x8000 ) ) {
			if ( ESC2_GPIO_Port == GPIO1st ) {
				motor_data_port1st[ i ] |= ESC2_Pin << 16;
			} else {
				motor_data_port2nd[ i ] |= ESC2_Pin << 16;
			}
		}
		if ( ! ( dshot_packet[ 2 ] & 0x8000 ) ) {
			if ( ESC3_GPIO_Port == GPIO1st ) {
				motor_data_port1st[ i ] |= ESC3_Pin << 16;
			} else {
				motor_data_port2nd[ i ] |= ESC3_Pin << 16;
			}
		}
		if ( ! ( dshot_packet[ 3 ] & 0x8000 ) ) {
			if ( ESC4_GPIO_Port == GPIO1st ) {
				motor_data_port1st[ i ] |= ESC4_Pin << 16;
			} else {
				motor_data_port2nd[ i ] |= ESC4_Pin << 16;
			}
		}

		dshot_packet[ 0 ] <<= 1;
		dshot_packet[ 1 ] <<= 1;
		dshot_packet[ 2 ] <<= 1;
		dshot_packet[ 3 ] <<= 1;
	}

	if ( ESC1_GPIO_Port == GPIO2nd || ESC2_GPIO_Port == GPIO2nd || ESC3_GPIO_Port == GPIO2nd || ESC4_GPIO_Port == GPIO2nd ) {
		dshot_dma_phase = 2;
	} else {
		dshot_dma_phase = 1;
	}

	dshot_dma_port1st();
}

#if defined(STM32F4)
void DMA2_Stream2_IRQHandler(void)
#elif defined(STM32F3)
void DMA1_Channel3_IRQHandler(void)
#else
#error
#endif
{
	extern DMA_HandleTypeDef hdma_tim1_ch2;
	HAL_DMA_IRQHandler( &hdma_tim1_ch2 );

	switch ( dshot_dma_phase ) {
		case 2:
			dshot_dma_phase = 1;
			dshot_dma_port2nd();
			break;
		case 1:
			dshot_dma_phase = 0;
			break;
		default:
			dshot_dma_phase = 0;
			break;
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
		dshot_dma_start();
	}
}

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
			dshot_dma_start();
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
		dshot_dma_start();
	}
	delay(DSHOT_COMMAND_DELAY_US);
}
#endif

#endif // DSHOT_DMA_DRIVER
