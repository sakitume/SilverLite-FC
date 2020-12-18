SPEED_OPTIMISED_SRC := ""
SIZE_OPTIMISED_SRC  := ""
NONE_OPTIMISED_SRC  := ""
DEBUG_OPTIMISED_SRC  := ""

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
#SOURCES =  \
#	$(wildcard $(BASE_DIR)/Core/Src/*.c) \
#	$(wildcard $(BASE_DIR)/SilverLite/*.c) \
#	$(wildcard $(BASE_DIR)/SilverLite/*.cpp) \
#	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.c) \
#	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.cpp) \

SOURCES =  \
	$(BASE_DIR)/Core/Src/drv_time.c \
	$(BASE_DIR)/Core/Src/drv_led.c \
	$(BASE_DIR)/Core/Src/drv_reset.c \
	$(BASE_DIR)/Test/test_basic_1_led.cpp \
	$(BASE_DIR)/SilverLite/delay.cpp \

#	$(BASE_DIR)/SilverLite/console.cpp \
#	$(BASE_DIR)/SilverLite/jee.cpp

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




#-------------------------------------------------------------------------------
# Source file groups
#-------------------------------------------------------------------------------
SPEED_OPTIMISED_SRC := $(SPEED_OPTIMISED_SRC) \

SIZE_OPTIMISED_SRC := $(SIZE_OPTIMISED_SRC) \

NONE_OPTIMISED_SRC := $(NONE_OPTIMISED_SRC) \

DEBUG_OPTIMISED_SRC := $(DEBUG_OPTIMISED_SRC) \
