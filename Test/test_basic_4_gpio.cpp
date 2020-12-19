#include "_my_config.h"
#include "console.h"
#include "jee.h"

extern "C" {
#include "drv_time.h"
#include "drv_reset.h"
}


//------------------------------------------------------------------------------
float looptime; // in seconds
uint32_t lastlooptime;
uint32_t used_loop_time;
uint32_t max_used_loop_time;

//------------------------------------------------------------------------------
#if defined(OMNIBUS)
static Pin<'C', 15> ledPin;
#endif


//------------------------------------------------------------------------------
static void silverlite_init()
{
    console_init();
    ledPin.mode(Pinmode::out);

    // If you don't perform a write(1) and only do a toggle(), you'll see that
    // toggle() fails to actually toggle
    // Try it yourself, disable the following line and you'll see the LED is
    // stuck on for the first 4 seconds (where it should be toggling), after
    // that 4 seconds we switch to using the write() method to toggle and then
    // 4 seconds later we go back to using toggle() and then toggle() works
    // correctly
    
//    ledPin.write(1);
}

//------------------------------------------------------------------------------
// This is called every LOOPTIME microseconds
static bool silverlite_update()
{
    // Make sure USB VCP is updated or risk losing connection
    console_poll();

    // Compute total elapsed milliseconds
    static uint32_t ticks;
    uint32_t ms =  ticks++ / (1000 / LOOPTIME);

    // Set starting mode to use Pin::toggle(), you'll see that the led is
    // stuck on
    static int mode = 1;

    // Every 500ms
    if (0 == (ms % 500))
    {
        
        // Every 4 seconds switch between direct writes versus toggle
        static int modeCntr;
        if (modeCntr++ >= 8)
        {
            modeCntr = 0;
            mode ^= 1;
        }

        static bool onOffState;
        if (onOffState)
        {
            switch (mode)
            {
                case 0:
                    ledPin.write(1);
                    break;
                case 1:
                    ledPin.toggle();
                    break;
            }
        }
        else
        {
            switch (mode)
            {
                case 0:
                    ledPin.write(0);
                    break;
                case 1:
                    ledPin.toggle();
                    break;
            }
        }
        onOffState = !onOffState;
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
        console_closePacket(0x05);

        secondTimer=  gettime();
        return true;    // true causes caller to reset max_used_loop_time
    }
    
    return false;
}

//------------------------------------------------------------------------------
extern "C" void usermain(void)
{
	delay( 1000 );
	time_init();

    silverlite_init();

	lastlooptime = gettime() - LOOPTIME;

	while ( true ) { // main loop
		const uint32_t loop_start_time = gettime();
		looptime = ( loop_start_time - lastlooptime ) * 1e-6f;
		lastlooptime = loop_start_time;

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
