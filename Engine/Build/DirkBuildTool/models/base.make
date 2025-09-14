## TARGET -- the target name, used for output files & dirs
## ROOT_DIR -- engine root dir
## TYPE -- shared, exec
## 
## CFLAGS -- flags for compilation like warnings and the standard
## CXXFLAGS -- more flags for the compilation (this one is mainly used for includes)
## DEFINES -- defines for compilation
## 
## LDFLAGS -- linker flags like lib dirs
## LDLIBS -- libs for linking

BIN_DIR=$(ROOT_DIR)/Binaries
INT_DIR=$(ROOT_DIR)/Intermediate/$(TARGET)

OUT=$(BIN_DIR)/
ifeq ($(TYPE), "shared")
        OUT+=lib$(TARGET).so
endif
ifeq ($(TYPE), "exec")
        OUT+=$(TARGET)
endif

LDFLAGS += -L$(BIN_DIR)
CXXFLAGS += -Iinclude

COMPILE_COMMANDS_FILE=$(INT_DIR)/compile_commands.json

SRC_DIR=src
SRC_EXT=.cpp
SRC=$(shell find $(SRC_DIR) -name '*$(SRC_EXT)')

SHADERS_DIR=shaders
SHADERS_OUT_DIR=$(INT_DIR)/shaders
SHADERS=$(shell find $(SHADERS_DIR) -type f)
SPV=$(SHADERS:$(SHADERS_DIR)/%=$(SHADERS_OUT_DIR)/%.spv)

OBJ_DIR=$(INT_DIR)/obj
OBJ=$(SRC:$(SRC_DIR)/%$(SRC_EXT)=$(OBJ_DIR)/%.o)

BUILD_CMD=$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) -MMD -MP -c $< -o $@

.PHONY: $(TARGET)
$(TARGET): $(OUT) $(SPV)
	@echo Built $(TARGET)

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%$(SRC_EXT)
	@echo Compiling $*$(SRC_EXT)...
	@mkdir -p $(dir $@)
	@echo '{"directory": "$(shell pwd)", "command": "$(BUILD_CMD)", "file": "$<"},' >> $(COMPILE_COMMANDS_FILE)
	@$(BUILD_CMD)

$(SHADERS_OUT_DIR)/%.spv: $(SHADERS_DIR)/%
	@echo Compiling shader $*...
	@mkdir -p $(dir $@)
	@glslc $< -o $@
