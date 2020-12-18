// https://www.st.com/resource/en/datasheet/stm32f303vc.pdf
// https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
// https://www.st.com/resource/en/reference_manual/dm00043574-stm32f303xb-c-d-e-stm32f303x6-8-stm32f328x8-stm32f358xc-stm32f398xe-advanced-arm-based-mcus-stmicroelectronics.pdf

/*
[Test\source.mk]
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/Test/test_basic_2_usb.cpp

#-------------------------------------------------------------------------------
# F3 USB support
#-------------------------------------------------------------------------------
SOURCES += \
	$(wildcard $(BASE_DIR)/Targets/OMNIBUS/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c) \
	$(wildcard $(BASE_DIR)/Targets/OMNIBUS/Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c) \
	$(wildcard $(BASE_DIR)/Targets/OMNIBUS/USB_DEVICE/App/*.c) \
	$(wildcard $(BASE_DIR)/Targets/OMNIBUS/USB_DEVICE/Target/*.c)

INCLUDE_DIRS+=\
	$(BASE_DIR)/Targets/OMNIBUS/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
	$(BASE_DIR)/Targets/OMNIBUS/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	$(BASE_DIR)/Targets/OMNIBUS/USB_DEVICE/App \
	$(BASE_DIR)/Targets/OMNIBUS/USB_DEVICE/Target


[Test\stm32cubemx.mk]

*/

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
    if ((now - lastTime) >= (1000 * 500))
    {
        lastTime = now;
        xprintf("Hello\n");

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
	time_init();
    console_init();

    while (true)
    {
        console_poll();
		loop();
    }
}
