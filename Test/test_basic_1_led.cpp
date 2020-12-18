// https://www.st.com/resource/en/datasheet/stm32f303vc.pdf
// https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
// https://www.st.com/resource/en/reference_manual/dm00043574-stm32f303xb-c-d-e-stm32f303x6-8-stm32f328x8-stm32f358xc-stm32f398xe-advanced-arm-based-mcus-stmicroelectronics.pdf
#include "_my_config.h"
#include "console.h"

extern "C" {
#include "drv_time.h"
#include "drv_reset.h"
#include "drv_led.h"
}


float looptime; // in seconds
uint32_t lastlooptime;
uint32_t used_loop_time;
uint32_t max_used_loop_time;

static void loop()
{
    static uint8_t onOffState;
    static uint32_t lastTime;
    uint32_t now = gettime();
#if 1
    if ((now - lastTime) >= (1000 * 500))
    {
        lastTime = now;
#else
    delay(1000 * 500);
    {
#endif
        if (onOffState)
        {
            ledoff();
        }
        else
        {
            ledon();
        }
        onOffState = !onOffState;
    }
}

extern "C" void usermain(void)
{
	ledoff();
	delay( 1000 );

    delay (1000 * 500);
    ledon();

	time_init();

	lastlooptime = gettime() - LOOPTIME;
    while (true)
    {
        ledflash( 100000, 12 );
//		loop();
    }
}
