# Setup Compiler --------------------------------------------------------------

CC   := arm-none-eabi-gcc
CXX  := arm-none-eabi-g++
SIZE := arm-none-eabi-size

# Config Compiler -------------------------------------------------------------

# Advanced build flags
FLAGS_BUILD := --specs=nano.specs 
FLAGS_BUILD += -ffunction-sections
FLAGS_BUILD += -fdata-sections
FLAGS_BUILD += -fstack-usage
FLAGS_BUILD += -Wall

# C++ Flags
FLAGS_CXX := -fno-exceptions
FLAGS_CXX += -fno-rtti
FLAGS_CXX += -fno-use-cxa-atexit
FLAGS_CXX += -MMD -MP

# Linker Flags
FLAGS_LD := -T"./$(LD_SCRIPT)"
FLAGS_LD += --specs=nano.specs
FLAGS_LD += -Wl,-Map="./${BUILD_DIR}/${PROJECT_NAME}.map"
FLAGS_LD += -Wl,--gc-sections 
FLAGS_LD += -static 
FLAGS_LD += -Wl,--start-group 
FLAGS_LD += -lc -lm -lstdc++ -lsupc++ 
FLAGS_LD += -Wl,--end-group