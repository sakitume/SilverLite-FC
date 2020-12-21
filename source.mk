SPEED_OPTIMISED_SRC := ""
SIZE_OPTIMISED_SRC  := ""
NONE_OPTIMISED_SRC  := ""
DEBUG_OPTIMISED_SRC  := ""

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
SOURCES =  \
	$(wildcard $(BASE_DIR)/Core/Src/*.c) \
	$(wildcard $(BASE_DIR)/SilverLite/*.c) \
	$(wildcard $(BASE_DIR)/SilverLite/*.cpp) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.c) \
	$(wildcard $(BASE_DIR)/SilverLite/rx_afhds2a/*.cpp) \

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

SIZE_OPTIMISED_SRC := $(SIZE_OPTIMISED_SRC) \

NONE_OPTIMISED_SRC := $(NONE_OPTIMISED_SRC) \

DEBUG_OPTIMISED_SRC := $(DEBUG_OPTIMISED_SRC) \
