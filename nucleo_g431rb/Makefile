#
# Makefile of the Firmware Example Project
#
# https://makefiletutorial.com/

PROJECT_NAME := franklyboot_nucleo_g4931rb

# Setup Compiler --------------------------------------------------------------

CC  := arm-none-eabi-gcc
CXX := arm-none-eabi-g++

DEFINES := STM32G431xx

C_VER		:= -std=gnu11
CPP_VER		:= -std=gnu++17

CORE_FLAGS		:= -mcpu=cortex-m4 -mthumb 
FPU_FLAGS 		:= -mfpu=fpv4-sp-d16 -mfloat-abi=hard
FLAGS 			:= --specs=nano.specs -ffunction-sections -fdata-sections -Wall -fstack-usage 
CPP_SETTINGS	:= -fno-exceptions -fno-rtti -fno-use-cxa-atexit -MMD -MP

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

INCLUDE_DIRS  = Core/Inc
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32G4xx/Include
INCLUDE_DIRS += Drivers/CMSIS/Include
INCLUDE_DIRS += ../../frankly-bootloader/include

SRCS_FILES = Core/Src/main.c
SRCS_FILES += Core/Src/bootloader_api.cpp
SRCS_FILES += Core/Startup/startup.S

LD_SCRIPT = STM32G431RBTX_FLASH.ld

# Create Build Flags ----------------------------------------------------------

CFLAGS 		:= $(C_VER) $(CORE_FLAGS) $(FPU_FLAGS) $(FLAGS)
CXXFLAGS	:= $(CPP_VER) $(CORE_FLAGS) $(FPU_FLAGS) $(FLAGS) $(CPP_SETTINGS)
LDFLAGS		:= $(CORE_FLAGS) $(FPU_FLAGS) -T"./STM32G431RBTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="./${BUILD_DIR}/${PROJECT_NAME}.map" -Wl,--gc-sections -static --specs=nano.specs -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group

# Append -D to the defines to create flags
DEF_FLAGS := $(addprefix -D,$(DEFINES))

# Append -I to the include directory
INC_FLAGS := $(addprefix -I./,$(INCLUDE_DIRS))

# Create objects which will be placed in the build folder
OBJS := $(SRCS_FILES:%=$(BUILD_DIR)/%.o)


# The final build step.
$(BUILD_DIR)/$(PROJECT_NAME): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build asm files
$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(DEF_FLAGS) $(INC_FLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -r $(BUILD_DIR)

.PHONY: info
info:
	@echo "[INCLUDES]: $(INC_FLAGS)"
	@echo "[OBJS]: $(OBJS)"
