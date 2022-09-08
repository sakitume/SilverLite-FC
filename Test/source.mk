SPEED_OPTIMISED_SRC := ""
SIZE_OPTIMISED_SRC  := ""
NONE_OPTIMISED_SRC  := ""
DEBUG_OPTIMISED_SRC  := ""

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
#TEST_CONFIG = test_basic_1_led
#TEST_CONFIG = test_basic_2_usb
#TEST_CONFIG = test_basic_3_adc
#TEST_CONFIG = test_basic_4_gpio
#TEST_CONFIG = test_4_osd
#TEST_CONFIG = test_5_ibus
#TEST_CONFIG = test_6_flysky
#TEST_CONFIG = test_7_fmc
TEST_CONFIG = test_9_crsf


ifeq ($(TEST_CONFIG), test_basic_1_led)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/Test/test_basic_1_led.cpp
else ifeq ($(TEST_CONFIG), test_basic_2_usb)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/Test/test_basic_2_usb.cpp
else ifeq ($(TEST_CONFIG), test_basic_3_adc)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/Test/test_basic_3_adc.cpp
else ifeq ($(TEST_CONFIG), test_basic_4_gpio)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/Test/test_basic_4_gpio.cpp
else ifeq ($(TEST_CONFIG), test_4_osd)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/Test/test_4_osd.cpp
else ifeq ($(TEST_CONFIG), test_5_ibus)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/Core/Src/util.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/SilverLite/rx_ibus.cpp \
	$(BASE_DIR)/SilverLite/jee.cpp \
	$(BASE_DIR)/Test/test_5_ibus.cpp
else ifeq ($(TEST_CONFIG), test_6_flysky)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/Core/Src/util.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/SilverLite/rx_flysky.cpp \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.c) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.cpp) \
	$(BASE_DIR)/SilverLite/jee.cpp \
	$(BASE_DIR)/Test/test_6_flysky.cpp
else ifeq ($(TEST_CONFIG), test_7_fmc)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/drv_fmc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/Core/Src/util.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/SilverLite/jee.cpp \
	$(BASE_DIR)/Test/test_7_fmc.cpp
else ifeq ($(TEST_CONFIG), test_9_elrs)
SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/Core/Src/util.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/SilverLite/drv_serial_rx.cpp \
	$(BASE_DIR)/SilverLite/rx_elrs.cpp \
	$(BASE_DIR)/SilverLite/jee.cpp \
	$(BASE_DIR)/Test/test_9_elrs.cpp
endif

#-------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------
INCLUDE_DIRS=\
	$(BASE_DIR)/Core/Src \
	$(BASE_DIR)/SilverLite \

#-------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------
DEFINES=\
	$(TARGET) \
	USE_SILVERLITE \


#-------------------------------------------------------------------------------
# F3 USB support
#-------------------------------------------------------------------------------
ifeq ($(IS_F3_TARGET), 1)
SOURCES += \
	$(wildcard $(BASE_DIR)/Targets/F3_USB_Support/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c) \
	$(wildcard $(BASE_DIR)/Targets/F3_USB_Support/Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c) \
	$(wildcard $(BASE_DIR)/Targets/F3_USB_Support/USB_DEVICE/App/*.c) \
	$(wildcard $(BASE_DIR)/Targets/F3_USB_Support/USB_DEVICE/Target/*.c)

INCLUDE_DIRS+=\
	$(BASE_DIR)/Targets/F3_USB_Support/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
	$(BASE_DIR)/Targets/F3_USB_Support/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	$(BASE_DIR)/Targets/F3_USB_Support/USB_DEVICE/App \
	$(BASE_DIR)/Targets/F3_USB_Support/USB_DEVICE/Target
endif


#-------------------------------------------------------------------------------
# Source file groups
#-------------------------------------------------------------------------------
SPEED_OPTIMISED_SRC := $(SPEED_OPTIMISED_SRC) \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Core/Src/drv_adc.c \
	$(BASE_DIR)/Core/Src/battery.c \
	$(BASE_DIR)/Core/Src/filter.c \
	$(BASE_DIR)/Core/Src/util.c \
	$(BASE_DIR)/SilverLite/delay.cpp \
	$(BASE_DIR)/SilverLite/f3_console.cpp \
	$(BASE_DIR)/SilverLite/console.cpp \
	$(BASE_DIR)/SilverLite/tprintf.cpp \
	$(BASE_DIR)/SilverLite/drv_osd.cpp \
	$(BASE_DIR)/SilverLite/jee.cpp \
	$(BASE_DIR)/Test/test_6_flysky.cpp \
	$(BASE_DIR)/SilverLite/rx_flysky.cpp \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/afhds2a.c) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/afhds2a_support.c) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/rx_a7105.c) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/rx_spi.cpp) \

SIZE_OPTIMISED_SRC := $(SIZE_OPTIMISED_SRC) \

NONE_OPTIMISED_SRC := $(NONE_OPTIMISED_SRC) \

DEBUG_OPTIMISED_SRC := $(DEBUG_OPTIMISED_SRC) \
