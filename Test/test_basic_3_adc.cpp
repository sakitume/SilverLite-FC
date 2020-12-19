#include "_my_config.h"
#include "console.h"

extern "C" {
#include "drv_time.h"
#include "drv_reset.h"
#include "drv_led.h"
#include "drv_adc.h"
#include "battery.h"
}


extern "C" {
    // Globals that battery.c references so we'll provide them here
	float thrsum;       // control.c
	int idle_offset;    // drv_dshot.c

    extern float vbattfilt; // battery.c
}


//------------------------------------------------------------------------------
float looptime; // in seconds
uint32_t lastlooptime;
uint32_t used_loop_time;
uint32_t max_used_loop_time;

//------------------------------------------------------------------------------
// This is called every LOOPTIME microseconds
bool silverlite_update()
{
    // Make sure USB VCP is updated or risk losing connection
    console_poll();

    // Compute total elapsed milliseconds
    static uint32_t ticks;
    uint32_t ms =  ticks++ / (1000 / LOOPTIME);

    // Every 100ms
    if (0 == (ms % 100))
    {
        // TODO: Placeholder, this is where we can do something every 10th of a second

        // return false to signal to main loop to not count this frame
        // when determining max loop time.
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
static bool silverlite_postupdate(uint32_t max_used_loop_time)
{
    static uint32_t secondTimer;
    if ((gettime() - secondTimer) >= 1000000)
    {
        console_openPacket();
        console_appendPacket16(max_used_loop_time);
        console_appendPacket16((uint16_t)(vbattfilt * 100));
        console_closePacket(0x05);

        secondTimer=  gettime();
        return true;    // true causes caller to reset max_used_loop_time
    }
    
    return false;
}

//------------------------------------------------------------------------------
extern "C" void usermain(void)
{
	ledoff();
	delay( 1000 );
	time_init();
	adc_init(); // DMA takes about 1 us once installed. Must be done early, adc is used in battery_init().
    console_init();
	battery_init(); // Must be called before gyro_cal() to send initial battery voltage there.

	lastlooptime = gettime() - LOOPTIME;

	while ( true ) { // main loop
		const uint32_t loop_start_time = gettime();
		looptime = ( loop_start_time - lastlooptime ) * 1e-6f;
		lastlooptime = loop_start_time;

		battery();

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
