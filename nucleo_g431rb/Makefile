#
# Makefile of the Firmware Example Project
#
# https://makefiletutorial.com/

## TODO -> Improve makefile
## FRANKLYBOOTLOADER is build ../../ direcotires due to relative path!!

PROJECT_NAME := franklyboot_nucleo_g4931rb

# Setup Compiler --------------------------------------------------------------

CC   := arm-none-eabi-gcc
CXX  := arm-none-eabi-g++
SIZE := arm-none-eabi-size

# Setup C-Version -------------------------------------------------------------


C_VER		:= -std=gnu11
CPP_VER		:= -std=gnu++17

# Configuration ---------------------------------------------------------------

CORE_FLAGS		:= -mcpu=cortex-m4 -mthumb
OPTIMIZATION    := -Os
FPU_FLAGS 		:= -mfpu=fpv4-sp-d16 -mfloat-abi=hard
FLAGS 			:= --specs=nano.specs -ffunction-sections -fdata-sections -Wall -fstack-usage 
CPP_SETTINGS	:= -fno-exceptions -fno-rtti -fno-use-cxa-atexit -MMD -MP

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := STM32G431xx

INCLUDE_DIRS  = Core/Inc
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32G4xx/Include
INCLUDE_DIRS += Drivers/CMSIS/Include
INCLUDE_DIRS += ../../frankly-bootloader/include

SRCS_FILES = Core/Src/main.c
SRCS_FILES += Core/Src/bootloader_api.cpp
SRCS_FILES += Core/Startup/startup.S
SRCS_FILES += ../../frankly-bootloader/src/francor/franklyboot/msg.cpp

LD_SCRIPT = STM32G431RBTX_FLASH.ld

# Create Build Flags ----------------------------------------------------------

CFLAGS 		:= $(C_VER) $(CORE_FLAGS) $(OPTIMIZATION) $(FPU_FLAGS) $(FLAGS)
CXXFLAGS	:= $(CPP_VER) $(CORE_FLAGS) $(OPTIMIZATION) $(FPU_FLAGS) $(FLAGS) $(CPP_SETTINGS)
LDFLAGS		:= $(CORE_FLAGS) $(OPTIMIZATION) $(FPU_FLAGS) -T"./STM32G431RBTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="./${BUILD_DIR}/${PROJECT_NAME}.map" -Wl,--gc-sections -static --specs=nano.specs -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group

# Append -D to the defines to create flags
DEF_FLAGS := $(addprefix -D,$(DEFINES))

# Append -I to the include directory
INC_FLAGS := $(addprefix -I./,$(INCLUDE_DIRS))

# Create objects which will be placed in the build folder
OBJS := $(SRCS_FILES:%=$(BUILD_DIR)/%.o)


all: pre-info $(BUILD_DIR)/$(PROJECT_NAME) size
	@echo ""
	@echo "==> Done"

pre-info:
	@echo "===================================================================="
	@echo "Building Firmware for $(PROJECT_NAME)"
	@echo "===================================================================="

# Final build step: Link all object files together
$(BUILD_DIR)/$(PROJECT_NAME): $(OBJS)
	@echo ""
	@echo "= Linking Firmware ================================================="
	@echo "[LD ] $< --> $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Create objects from ASM
$(BUILD_DIR)/%.S.o: %.S
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $@)
	@echo "[ASM]: $< --> $(OUT)"
	@$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

# Create objects from C
$(BUILD_DIR)/%.c.o: %.c
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $@)
	@echo "[CC ]: $< --> $(subst ../,,$@)"
	@$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

# Create objects from CPP
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $@)
	@echo "[CXX]: $< --> $(subst ../,,$@)"
	@$(CXX) $(DEF_FLAGS) $(INC_FLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	@rm -r $(BUILD_DIR)

.PHONY: size
size:
	@echo ""
	@echo "= Firmware Size ====================================================="
	@$(SIZE) $(BUILD_DIR)/$(PROJECT_NAME)
	@echo "===================================================================="


