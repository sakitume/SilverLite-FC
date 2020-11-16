SPEED_OPTIMISED_SRC := ""
SIZE_OPTIMISED_SRC  := ""
NONE_OPTIMISED_SRC  := ""
DEBUG_OPTIMISED_SRC  := ""

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
SOURCES =  \
	$(wildcard Core/Src/*.c) \
	$(wildcard SilverLite/*.c) \
	$(wildcard SilverLite/*.cpp) \
	$(wildcard SilverLite/rx_afhds2a/*.c) \
	$(wildcard SilverLite/rx_afhds2a/*.cpp) \

#-------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------
INCLUDE_DIRS=\
	Core/Src \
	SilverLite \

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
