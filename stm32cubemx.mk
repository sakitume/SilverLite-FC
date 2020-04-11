#-------------------------------------------------------------------------------
# STM32CubeMX generated sources
#-------------------------------------------------------------------------------
ifeq ($(TARGET),OMNIBUSF4)
SOURCES +=  \
	startup_stm32f405xx.s
else ifeq ($(TARGET),NOX)
SOURCES +=  \
	startup_stm32f411xe.s
else
	$(error TARGET must be either OMNIBUSF4 or NOX, it is: $(TARGET))
endif

SOURCES +=  \
	Targets/$(TARGET)/Core/Src/main.c \
	Targets/$(TARGET)/Core/Src/stm32f4xx_it.c \
	Targets/$(TARGET)/Core/Src/stm32f4xx_hal_msp.c \
	Targets/$(TARGET)/Core/Src/system_stm32f4xx.c \

#-------------------------------------------------------------------------------
# STM32CubeMX generated includes
#-------------------------------------------------------------------------------
INCLUDE_DIRS +=  \
	Targets/$(TARGET)/Core/Inc \

#-------------------------------------------------------------------------------
# STM32 sources
#-------------------------------------------------------------------------------
SOURCES +=  $(wildcard Drivers/STM32F4xx_HAL_Driver/Src/*.c)

#-------------------------------------------------------------------------------
# STM32 Defines
#-------------------------------------------------------------------------------
DEFINES += \
	USE_HAL_DRIVER \

ifeq ($(TARGET),OMNIBUSF4)
DEFINES += \
	STM32F405xx
else ifeq ($(TARGET),NOX)
DEFINES += \
	STM32F411xE	
else
	$(error TARGET must be either OMNIBUSF4 or NOX, it is: $(TARGET))
endif

#-------------------------------------------------------------------------------
# STM32 Include directories
#-------------------------------------------------------------------------------
INCLUDE_DIRS += \
	Drivers/STM32F4xx_HAL_Driver/Inc \
	Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	Drivers/CMSIS/Include \

#-------------------------------------------------------------------------------
# STM32 link script
#-------------------------------------------------------------------------------
ifeq ($(TARGET),OMNIBUSF4)
LDSCRIPT = STM32F405RGTx_FLASH.ld
else ifeq ($(TARGET),NOX)
LDSCRIPT = STM32F411CEUx_FLASH.ld
else
	$(error TARGET must be either OMNIBUSF4 or NOX, it is: $(TARGET))
endif
