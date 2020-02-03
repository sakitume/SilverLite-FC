#include <stdint.h>

extern "C" {
    extern uint32_t gettime( void ); // return time in micro seconds from start
    extern void delay( uint32_t us );
}

//------------------------------------------------------------------------------
void delay_ms(uint32_t ms)
{
    delay(ms * 1000);
}

//------------------------------------------------------------------------------
void delay_us(uint32_t us)	
{	
    delay(us);
}


