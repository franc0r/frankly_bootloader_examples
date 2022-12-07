# Create Build Flags ----------------------------------------------------------

CFLAGS 		:= $(C_VER) $(FLAGS_CORE) $(FLAGS_OPTIM) $(FLAGS_BUILD)
CXXFLAGS	:= $(CXX_VER) $(FLAGS_CORE) $(FLAGS_OPTIM) $(FLAGS_BUILD) $(FLAGS_CXX)
LDFLAGS		:= $(FLAGS_CORE) $(FLAGS_OPTIM) $(FLAGS_LD)
 

# Append -D to the defines to create flags
DEF_FLAGS := $(addprefix -D,$(DEFINES))

# Append -I to the include directory
INC_FLAGS := $(addprefix -I./,$(INCLUDE_DIRS))

# Create objects which will be placed in the build folder
OBJS := $(SRCS_FILES:%=$(BUILD_DIR)/%.o)

# Build -----------------------------------------------------------------------

all: pre-info $(BUILD_DIR)/$(PROJECT_NAME) size
	@echo ""
	@echo "==> Done"

pre-info:
	@echo "===================================================================="
	@echo "Building Firmware for $(PROJECT_NAME)"
	@echo "===================================================================="

# Final build step: Link all object files together
$(BUILD_DIR)/$(PROJECT_NAME): $(OBJS)
	@echo "[LD ]: Linking $(notdir $@)"
	@$(CXX) $(subst ../,,$(OBJS)) -o $@ $(LDFLAGS)

# Create objects from ASM
$(BUILD_DIR)/%.S.o: %.S
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $(OUT))
	@echo "[ASM]: $<"
	@$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $(OUT)

# Create objects from C
$(BUILD_DIR)/%.c.o: %.c
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $(OUT))
	@echo "[CC ]: $<"
	@$(CC) $(DEF_FLAGS) $(INC_FLAGS) $(CFLAGS) -c $< -o $(OUT)

# Create objects from CPP
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(eval OUT := $(subst ../,,$@))
	@mkdir -p $(dir $(OUT))
	@echo "[CXX]: $<"
	@$(CXX) $(DEF_FLAGS) $(INC_FLAGS) $(CXXFLAGS) -c $< -o $(OUT)


.PHONY: clean
clean:
	@rm -r $(BUILD_DIR)

.PHONY: size
size:
	@echo ""
	@echo "--------------------------------------------------------------------"
	@$(SIZE) $(BUILD_DIR)/$(PROJECT_NAME)
	@echo "--------------------------------------------------------------------"