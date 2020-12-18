// https://www.st.com/resource/en/datasheet/stm32f303vc.pdf
// https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
// https://www.st.com/resource/en/reference_manual/dm00043574-stm32f303xb-c-d-e-stm32f303x6-8-stm32f328x8-stm32f358xc-stm32f398xe-advanced-arm-based-mcus-stmicroelectronics.pdf

/*
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Test/test_basic_1_led.cpp \
	$(BASE_DIR)/SilverLite/delay.cpp \
*/

#include "_my_config.h"

extern "C" {
#include "drv_time.h"
#include "drv_reset.h"
#include "drv_led.h"
}

//------------------------------------------------------------------------------
static void test_flash()
{
    ledflash( 100000, 12 );
}

//------------------------------------------------------------------------------
static void test_delay()
{
    static uint8_t onOffState;
    delay(1000 * 500);
    {
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

//------------------------------------------------------------------------------
static void test_gettime()
{
    static uint8_t onOffState;
    static uint32_t lastTime;
    uint32_t now = gettime();
    if ((now - lastTime) >= (1000 * 500))
    {
        lastTime = now;

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
	delay(1000);
	time_init();

    int testNum = 0;
    while (true)
    {
        switch (testNum)
        {
            case 0:
                test_flash();
                break;
            
            case 1:
                test_delay();
                break;
            
            case 2:
                test_gettime();
                break;
            
            default:
                break;
        }
    }
}
