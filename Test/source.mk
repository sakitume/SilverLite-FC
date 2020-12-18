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
	$(BASE_DIR)/Test/test_usb.cpp \
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
# Source file groups
#-------------------------------------------------------------------------------
SPEED_OPTIMISED_SRC := $(SPEED_OPTIMISED_SRC) \

SIZE_OPTIMISED_SRC := $(SIZE_OPTIMISED_SRC) \

NONE_OPTIMISED_SRC := $(NONE_OPTIMISED_SRC) \

DEBUG_OPTIMISED_SRC := $(DEBUG_OPTIMISED_SRC) \
