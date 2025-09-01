TARGET=Runtime
BIN_DIR=../../../Binaries
INT_DIR=../../../Intermediate/$(TARGET)
THIRDPARTY_DIR=../Thirdparty
INCLUDE_DIR=include

CXX_STD=c++23

OUT=$(BIN_DIR)/$(TARGET).so

SRC_DIR=src
SRC_EXT=.cpp
SRC=$(shell find $(SRC_DIR) -name '*$(SRC_EXT)')

OBJ_DIR=$(INT_DIR)/obj
OBJ=$(SRC:$(SRC_DIR)/%$(SRC_EXT)=$(OBJ_DIR)/%.o)

CFLAGS=-Wall -Wextra -std=$(CXX_STD)
CXXFLAGS=-I$(INCLUDE_DIR) -I$(THIRDPARTY_DIR)/glm -I$(THIRDPARTY_DIR) -I${GLFW}/include -I${VULKAN_SDK}/include
DEFINES=-DGLFW_INCLUDE_NONE -DGLFW_INCLUDE_VULKAN -DLOG_PATH=\"./\" -DRESOURCE_PATH=\"./\" -DSHADER_PATH=\"./\"

LDFLAGS=-L${GLFW}/lib -L${VULKAN_SDK}/lib
LDLIBS=-lglfw -lvulkan

.PHONY: $(TARGET)
$(TARGET): $(OUT)

$(OUT): $(OBJ)
	@mkdir -p $(dir $(OUT))
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $(OUT)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%$(SRC_EXT)
	@echo $*$(SRC_EXT)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) -MMD -MP -c $< -o $@
