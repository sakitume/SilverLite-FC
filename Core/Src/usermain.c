#include <stdbool.h>
#include <stdint.h>

#include "battery.h"
#include "blackbox.h"
#include "config.h"
#include "control.h"
#include "drv_adc.h"
#include "drv_dshot.h"
#include "drv_led.h"
#include "drv_time.h"
#include "flash.h"
#include "gestures.h"
#include "imu.h"
#include "led.h"
#include "rx.h"
#include "sixaxis.h"

#include "debug.h"

#ifdef USE_SILVERLITE
#include "silverlite.h"
#endif

float looptime; // in seconds
uint32_t lastlooptime;
uint32_t used_loop_time;
uint32_t max_used_loop_time;
bool telemetry_transmitted;

extern char aux[ AUXNUMBER ];
extern int onground; // control.c

void failloop( int val );

void usermain()
{
	ledoff();
	delay( 1000 );

	time_init();
	pwm_init(); // For legacy reasons it's called pwm even in the Dshot driver.
	sixaxis_init();
	adc_init(); // DMA takes about 1 us once installed. Must be done early, adc is used in battery_init().
	flash_calculate_pid_c_identifier(); // Must be called before flash_load().
#ifdef USE_SILVERLITE
	// silverlite_init() must be called before flash_load() so PIDs defined by _my_config.h can 
	// be initialized into pidkp[], pidki[], pidkd[] and be checked against by flash_load
	silverlite_init();	
#endif
	flash_load(); // Must be called before rx_init() for autobind to work.
	rx_init();
	battery_init(); // Must be called before gyro_cal() to send initial battery voltage there.
	gyro_cal();
	imu_init();
	blackbox_init();

	lastlooptime = gettime() - LOOPTIME;

	while ( true ) { // main loop
		// Time measurements with ARMCLANG -O3:
		// sixaxis_read(): 11 +2 per gyro filter (contains 10 SPI bitbang time)
		// control(): 23 acro (+3 angle)
		// checkrx(): 17 us worst case for LOOPTIME < 1000; 39 us otherwise [+1 in case of nrf24 scrambling]

		const uint32_t loop_start_time = gettime();
		looptime = ( loop_start_time - lastlooptime ) * 1e-6f;
		lastlooptime = loop_start_time;

		static uint32_t last_accel_read_time;
		if ( telemetry_transmitted // This way we sync gyro reading to the subsequent loop after sending telemetry.
			|| loop_start_time - last_accel_read_time > 950 ) // The accel sensor gets updated only once every 1 ms.
		{
			telemetry_transmitted = false;
			last_accel_read_time = loop_start_time;
			sixaxis_read(); // read gyro (and accelerometer data for blackbox logging)
		} else {
			gyro_read(); // read just gyro data
		}
		// Gyro filtering is done in sixaxis.c
#ifdef LEVELMODE
		imu(); // attitude calculations for level mode
#endif // LEVELMODE
		control(true); // all flight calculations and motors
		blackbox_log();
		battery();
		if ( onground ) {
			gestures(); // check gestures
		}
		process_led_command();
		checkrx(); // receiver function

#ifdef USE_SILVERLITE
		if (silverlite_update()) {
#endif
		// for debug
		used_loop_time = gettime() - loop_start_time;
		if ( used_loop_time > max_used_loop_time ) {
			max_used_loop_time = used_loop_time;
		}
#ifdef USE_SILVERLITE
		}
#endif

		static uint32_t next_loop_start = 0;
		if ( next_loop_start == 0 ) {
			next_loop_start = loop_start_time;
		}
		next_loop_start += LOOPTIME;

#ifdef USE_SILVERLITE
		if (silverlite_postupdate(max_used_loop_time))
		{
            max_used_loop_time = 0;
		}
#endif
		while ( gettime() < next_loop_start );
	}
}

#ifdef USE_SILVERLITE
// failloop() uses long delays which will cause our USB VCP to timeout
// so this is a little hack to prevent that from happening
static void delayWithPolling(uint32_t us)
{
	while (us >= 1000)
	{
		delay(1000);
		us -= 1000;
		silverlite_poll();
	}
	if (us)
	{
		delay(us);
	}
}
#define 	delay	delayWithPolling
#endif


// 2 - low battery at powerup - if enabled by config
// 3 - radio chip not detected
// 4 - Gyro not found
// 5 - clock, interrupts, systick, bad code
// 6 - flash write error
// 7 - ESC pins on more than two distinct GPIO ports
void failloop( int val )
{
	for ( int i = 0; i <= 3; ++i ) {
		pwm_set( i, 0 );
	}
	while ( true ) {
		for ( int i = 0; i < val; ++i ) {
			ledon();
			delay( 200000 );
			ledoff();
			delay( 200000 );
		}
		delay( 800000 );
	}
}

void HardFault_Handler( void )
{
	failloop( 5 );
}

void MemManage_Handler( void )
{
	failloop( 5 );
}

void BusFault_Handler( void )
{
	failloop( 5 );
}

void UsageFault_Handler( void )
{
	failloop( 5 );
}
