#-------------------------------------------------------------------------------
# Provide default TARGET if none was specified
# Determine if target is F3 or F4 class processor (also verifies TARGET is valid)
#-------------------------------------------------------------------------------
TARGET ?= OMNIBUS

F4_TARGETS := OMNIBUSF4 NOX MATEKF411RX
F3_TARGETS := OMNIBUS CRAZYBEEF3FS

IS_F4_TARGET = 0
IS_F3_TARGET = 0

# if the result of searching for $(TARGET) in $(F4_TARGETS) is not empty
ifneq ($(filter $(TARGET),$(F4_TARGETS)),)
IS_F4_TARGET = 1
else ifneq ($(filter $(TARGET),$(F3_TARGETS)),)
IS_F3_TARGET = 1
else
$(error TARGET must be in: $(F4_TARGETS) $(F3_TARGETS))
endif

#-------------------------------------------------------------------------------
# STM32CubeMXProgrammer
# To install on Mac OS I had to download and unzip the package and run:
# java -jar SetupSTM32CubeProgrammer-2.4.0.exe
# This launched the installer executable
#-------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
	STM_CUBE_PROGRAMMER = STM32_Programmer_CLI.exe
else
	STM_CUBE_PROGRAMMER = /Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/MacOs/bin/STM32_Programmer_CLI
endif

#-------------------------------------------------------------------------------
# GNU ARM Embedded Toolchain
#-------------------------------------------------------------------------------
CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
LD=arm-none-eabi-ld
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump
NM=arm-none-eabi-nm
SIZE=arm-none-eabi-size
A2L=arm-none-eabi-addr2line

#-------------------------------------------------------------------------------
# Working directories
#-------------------------------------------------------------------------------
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	OBJECT_DIR	= gcc_debug/$(TARGET)
else
	OBJECT_DIR	= gcc_release/$(TARGET)
endif

#-------------------------------------------------------------------------------
# Source files (SOURCES), Include folders (INCLUDE_DIRS), Defines (DEFINES)
#-------------------------------------------------------------------------------
#BASE_DIR 	= .
BASE_DIR 	= ..
include source.mk
include stm32cubemx.mk

#-------------------------------------------------------------------------------
# Object List
#-------------------------------------------------------------------------------
OBJECTS=$(addsuffix .o,$(addprefix $(OBJECT_DIR)/,$(basename $(SOURCES))))

DEPS=$(addsuffix .d,$(addprefix $(OBJECT_DIR)/,$(basename $(SOURCES))))

#-------------------------------------------------------------------------------
# Target Output Files
#-------------------------------------------------------------------------------
TARGET_ELF=$(OBJECT_DIR)/$(TARGET).elf
TARGET_HEX=$(OBJECT_DIR)/$(TARGET).hex
TARGET_BIN=$(OBJECT_DIR)/$(TARGET).bin


#-------------------------------------------------------------------------------
# Flags
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Default Tool options - can be overridden in {mcu}.mk files.
#
CC_DEBUG_OPTIMISATION   := -ggdb3 -Og -DDEBUG 
CC_DEFAULT_OPTIMISATION := -O2
CC_SPEED_OPTIMISATION   := -Ofast
CC_SIZE_OPTIMISATION    := -Os
CC_NONE_OPTIMISATION    := -O0

# compile gcc flags
CFLAGS = $(MCU) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDE_DIRS)) -Wall -Wextra -Wunsafe-loop-optimizations -Wdouble-promotion -fdata-sections -ffunction-sections

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# C++ compile flags are just lie C flags but with some additional items
CXXFLAGS= $(CFLAGS) -fno-rtti -fno-exceptions -U__STRICT_ANSI__ 
ASMFLAGS= -x assembler-with-cpp 

#-------------------------------------------------------------------------------
# LDFLAGS
#######################################

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) --specs=nano.specs --specs=nosys.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(OBJECT_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

#-------------------------------------------------------------------------------
# Build
#
#	$@ The name of the target file (the one before the colon)
#	$< The name of the first (or only) prerequisite file (the first one after the colon)
#	$^ The names of all the prerequisite files (space separated)
#	$* The stem (the bit which matches the % wildcard in the rule definition.
#
#-------------------------------------------------------------------------------
$(TARGET_HEX): $(TARGET_ELF) $(TARGET_BIN)
	$(CP) -O ihex --set-start 0x0000000 $< $@

$(TARGET_BIN): $(TARGET_ELF)
	$(CP) -O binary $< $@

$(TARGET_ELF): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	$(SIZE) $(TARGET_ELF)

# Compile
SEPARATE_BUILDS ?= 1
ifeq ($(SEPARATE_BUILDS), 1)

ifeq ($(DEBUG), 1)
$(OBJECT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "%% (debug) $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_DEBUG_OPTIMISATION) $<
else
$(OBJECT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(if $(findstring $<,$(SPEED_OPTIMISED_SRC)), \
	echo "%% (speed optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_SPEED_OPTIMISATION) $<, \
	$(if $(findstring $<,$(SIZE_OPTIMISED_SRC)), \
	echo "%% (size optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_SIZE_OPTIMISATION) $<, \
	$(if $(findstring $<,$(NONE_OPTIMISED_SRC)), \
	echo "%% (not optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_NONE_OPTIMISATION) $<, \
	$(if $(findstring $<,$(DEBUG_OPTIMISED_SRC)), \
	echo "%% (debug optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_DEBUG_OPTIMISATION) $<, \
	echo "%% $(notdir $<)" "$(STDOUT)" && \
	$(CXX) -c -o $@ $(CXXFLAGS) $(CC_DEFAULT_OPTIMISATION) $<))))
endif

ifeq ($(DEBUG), 1)
$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "%% (debug) $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_DEBUG_OPTIMISATION) $<
else
$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(if $(findstring $<,$(SPEED_OPTIMISED_SRC)), \
	echo "%% (speed optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_SPEED_OPTIMISATION) $<, \
	$(if $(findstring $<,$(SIZE_OPTIMISED_SRC)), \
	echo "%% (size optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_SIZE_OPTIMISATION) $<, \
	$(if $(findstring $<,$(NONE_OPTIMISED_SRC)), \
	echo "%% (not optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_NONE_OPTIMISATION) $<, \
	$(if $(findstring $<,$(DEBUG_OPTIMISED_SRC)), \
	echo "%% (debug optimised) $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_DEBUG_OPTIMISATION) $<, \
	echo "%% $(notdir $<)" "$(STDOUT)" && \
	$(CC) -c -o $@ $(CFLAGS) $(CC_DEFAULT_OPTIMISATION) $<))))
endif

else

$(OBJECT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CXX) -c -o $@ $(CXXFLAGS) $(CC_DEFAULT_OPTIMISATION) $<

$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(CFLAGS) $(CC_DEFAULT_OPTIMISATION) $<

endif


$(OBJECT_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASMFLAGS) $<

$(OBJECT_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASMFLAGS) $<

#-------------------------------------------------------------------------------
# Recipes
#-------------------------------------------------------------------------------
.PHONY: all flash clean

clean:
	rm -rf $(OBJECTS) $(DEPS) $(TARGET_ELF) $(TARGET_HEX) $(OBJECT_DIR)/output.map


ifeq ($(TARGET),OMNIBUSF4)
flash: $(TARGET_ELF)
	openocd -d2 -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f4x.cfg -c "reset_config none" -c "program $(TARGET_ELF)  verify reset; shutdown;"
else
flash: $(TARGET_ELF)
#	STM32_Programmer_CLI.exe --connect port=USB1 -w $(TARGET_ELF) --start
	$(STM_CUBE_PROGRAMMER) --connect port=USB1 -w $(TARGET_ELF) --start
endif

# Flash using ST-Link adapter
stlinkflash: $(TARGET_ELF)
	openocd -d2 -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f4x.cfg -c "reset_config none" -c "program $(TARGET_ELF)  verify reset; shutdown;"

# Flash F3 device using ST-Link adapter
stlinkflashf3: $(TARGET_ELF)
	openocd -d2 -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f3x.cfg -c "reset_config none" -c "program $(TARGET_ELF)  verify reset; shutdown;"


# Flash using STM32CubeProgrammer via USB
download: $(TARGET_ELF)
#	STM32_Programmer_CLI.exe --connect port=USB1 -w $(TARGET_ELF) --start
#	/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/MacOs/bin/STM32_Programmer_CLI  --connect port=USB1 -w $(TARGET_ELF) --start
	$(STM_CUBE_PROGRAMMER) --connect port=USB1 -w $(TARGET_ELF) --start

# Choose one of the following "all"
#all: $(TARGET_BIN)
all: $(TARGET_HEX)

$(DEPS):

include $(wildcard $(DEPS))

