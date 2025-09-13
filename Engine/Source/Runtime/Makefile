TARGET=Runtime
CXX_STD=c++23

BIN_DIR=../../../Binaries
INT_DIR=../../../Intermediate/$(TARGET)
THIRDPARTY_DIR=../Thirdparty
COMPILE_COMMANDS_FILE=../../../compile_commands.json

# these are relative to the project root
WORKING_DIRECTORY=Engine/Source/$(TARGET)
ROOT_INT_DIR=Intermediate/$(TARGET)
ROOT_SAVE_DIR=Saved/$(TARGET)

## SHOULD DEPEND ON ISLIB
OUT=$(BIN_DIR)/lib$(TARGET).so

CFLAGS=-fPIC -Wall -Wextra -std=$(CXX_STD)
CXXFLAGS=-I$(INCLUDE_DIR) -I$(THIRDPARTY_DIR)/glm -I$(THIRDPARTY_DIR)/tinygltf -I${GLFW}/include -I${VULKAN_SDK}/include
DEFINES=-DGLFW_INCLUDE_NONE -DGLFW_INCLUDE_VULKAN -DLOG_PATH=\"$(ROOT_SAVE_DIR)/logs\" -DRESOURCE_PATH=\"$(WORKING_DIRECTORY)/resources\" -DSHADER_PATH=\"$(ROOT_INT_DIR)/shaders\"

LDFLAGS=-shared -L${GLFW}/lib -L${VULKAN_SDK}/lib
LDLIBS=-lglfw -lvulkan



INCLUDE_DIR=include
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
