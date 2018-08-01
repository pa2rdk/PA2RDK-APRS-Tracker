################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -f

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include libraries/Wire/src/utility/subdir.mk
-include libraries/Wire/src/subdir.mk
-include libraries/TinyGPSPlus-master/subdir.mk
-include libraries/TimerOne-r11/subdir.mk
-include libraries/SoftwareSerial/src/subdir.mk
-include libraries/RDKAPRS/subdir.mk
-include libraries/LiquidCrystal_SSD1306-master/subdir.mk
-include core/core/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
AR += \
/Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/arduino.ar \

ELF += \
RDKTracker.elf \

HEX += \
RDKTracker.hex \


# All Target
all: RDKTracker

# Tool invocations
RDKTracker: $(HEX)
	@echo 'Building target: $@'
	@echo 'Printing size:'
	"/Applications/Eclipse/arduino-oxygen/Eclipse.app/Contents/MacOS//arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-size" -A "/Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/RDKTracker.elf"
	@echo 'Finished building target: $@'
	@echo ' '

/Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/arduino.ar: $(AR_OBJ)
	@echo 'Starting archiver'
	"/Applications/Eclipse/arduino-oxygen/Eclipse.app/Contents/MacOS//arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-gcc-ar" rcs    /Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/arduino.ar $(AR_OBJ)
	@echo 'Finished building: $@'
	@echo ' '

RDKTracker.elf: $(LINK_OBJ) $(AR)
	@echo 'Starting combiner'
	"/Applications/Eclipse/arduino-oxygen/Eclipse.app/Contents/MacOS//arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-gcc" -Wall -Wextra -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections -mmcu=atmega328p -o "/Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/RDKTracker.elf"   $(LINK_OBJ)   /Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release/arduino.ar  "-L/Users/robertdekok/Dropbox/Arduino-workspace/RDKTracker/Release" -lm
	@echo 'Finished building: $@'
	@echo ' '

RDKTracker.hex: $(ELF)
	@echo 'Create eeprom image'
	${A.RECIPE.OBJCOPY.EEP.PATTERN}
	${A.RECIPE.OBJCOPY.HEX.PATTERN}
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(PDE_DEPS)$(C_DEPS)$(AR)$(CC_DEPS)$(AR_OBJ)$(C++_DEPS)$(LINK_OBJ)$(CXX_DEPS)$(ASM_DEPS)$(HEX)$(INO_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(ELF)$(CPP_DEPS)
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
