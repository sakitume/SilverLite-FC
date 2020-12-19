#-------------------------------------------------------------------------------
# Determine if target is F3 or F4 class processor
#-------------------------------------------------------------------------------
IS_F4_TARGET = 0
IS_F3_TARGET = 0
F4_TARGETS := OMNIBUSF4 NOX MATEKF411RX
F3_TARGETS := OMNIBUS CRAZYBEEF3FS

# if the result of searching for $(TARGET) in $(F4_TARGETS) is not empty
ifneq ($(filter $(TARGET),$(F4_TARGETS)),)
IS_F4_TARGET = 1
else ifneq ($(filter $(TARGET),$(F3_TARGETS)),)
IS_F3_TARGET = 1
else
#$(error TARGET must be in: $(F4_TARGETS) $(F3_TARGETS))
endif

#-------------------------------------------------------------------------------
# STM32CubeMX generated/provided startup file
#-------------------------------------------------------------------------------
ifneq ($(filter $(TARGET),NOX MATEKF411RX),)
SOURCES +=  \
	$(BASE_DIR)/startup_stm32f411xe.s
else ifneq ($(filter $(TARGET),OMNIBUS CRAZYBEEF3FS),)
SOURCES +=  \
	$(BASE_DIR)/startup_stm32f303cctx.s
else ifeq ($(TARGET),OMNIBUSF4)
SOURCES +=  \
	$(BASE_DIR)/startup_stm32f405xx.s
else
$(error TARGET must be in: $(F4_TARGETS) $(F3_TARGETS))
endif

ifeq ($(IS_F4_TARGET), 1)
#-- F4 targets are expected to have these STM32CubeMX generated source files
SOURCES +=  \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/main.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/stm32f4xx_it.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/stm32f4xx_hal_msp.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/system_stm32f4xx.c
else
#-- F3 targets are expected to have these STM32CubeMX generated source files
SOURCES +=  \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/main.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/stm32f3xx_it.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/stm32f3xx_hal_msp.c \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Src/system_stm32f3xx.c
endif

#-------------------------------------------------------------------------------
# STM32CubeMX generated includes
#-------------------------------------------------------------------------------
INCLUDE_DIRS +=  \
	$(BASE_DIR)/Targets/$(TARGET)/Core/Inc \

#-------------------------------------------------------------------------------
# STM32 sources
#-------------------------------------------------------------------------------
ifeq ($(IS_F4_TARGET), 1)
#-- F4 targets are expected to use these STM32CubeMX provided source files
SOURCES +=  $(wildcard $(BASE_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/*.c)
else
#-- F3 targets are expected to use these STM32CubeMX provided source files
SOURCES +=  $(wildcard $(BASE_DIR)/Drivers/STM32F3xx_HAL_Driver/Src/*.c)
endif

#-------------------------------------------------------------------------------
# STM32 Defines
#-------------------------------------------------------------------------------
DEFINES += \
	USE_HAL_DRIVER \

ifeq ($(TARGET),OMNIBUSF4)
DEFINES += \
	STM32F405xx \
	STM32F4
else ifneq ($(filter $(TARGET),NOX MATEKF411RX),)
DEFINES += \
	STM32F411xE	\
	STM32F4
else ifneq ($(filter $(TARGET),OMNIBUS CRAZYBEEF3FS),)
DEFINES += \
	STM32F303xC \
	STM32F3
else
$(error TARGET must be in: $(F4_TARGETS) $(F3_TARGETS))
endif

#-------------------------------------------------------------------------------
# STM32 Include directories
#-------------------------------------------------------------------------------
ifeq ($(IS_F4_TARGET), 1)
#-- F4 targets are expected to use these folders
INCLUDE_DIRS += \
	$(BASE_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc \
	$(BASE_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	$(BASE_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
else
#-- F3 targets are expected to use these folders
INCLUDE_DIRS += \
	$(BASE_DIR)/Drivers/STM32F3xx_HAL_Driver/Inc \
	$(BASE_DIR)/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy \
	$(BASE_DIR)/Drivers/CMSIS/Device/ST/STM32F3xx/Include
endif
INCLUDE_DIRS += \
	$(BASE_DIR)/Drivers/CMSIS/Include

#-------------------------------------------------------------------------------
# STM32 link script
#-------------------------------------------------------------------------------
ifeq ($(TARGET),OMNIBUSF4)
LDSCRIPT = $(BASE_DIR)/STM32F405RGTx_FLASH.ld
else ifneq ($(filter $(TARGET),NOX MATEKF411RX),)
LDSCRIPT = $(BASE_DIR)/STM32F411CEUx_FLASH.ld
else ifneq ($(filter $(TARGET),OMNIBUS CRAZYBEEF3FS),)
LDSCRIPT = $(BASE_DIR)/STM32F303CCTX_FLASH.ld
else
$(error TARGET must be in: $(F4_TARGETS) $(F3_TARGETS))
endif
