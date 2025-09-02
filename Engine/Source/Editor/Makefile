TARGET=Editor
BIN_DIR=../../../Binaries
INT_DIR=../../../Intermediate/$(TARGET)
THIRDPARTY_DIR=../Thirdparty
RUNTIME_DIR=../Runtime
INCLUDE_DIR=include

CXX_STD=c++23

OUT=$(BIN_DIR)/$(TARGET)

SRC_DIR=src
SRC_EXT=.cpp
SRC=$(shell find $(SRC_DIR) -name '*$(SRC_EXT)')

OBJ_DIR=$(INT_DIR)/obj
OBJ=$(SRC:$(SRC_DIR)/%$(SRC_EXT)=$(OBJ_DIR)/%.o)

CFLAGS=-fPIC -Wall -Wextra -std=$(CXX_STD)
CXXFLAGS=-I$(INCLUDE_DIR) -I$(THIRDPARTY_DIR)/glm -I$(THIRDPARTY_DIR) -I${GLFW}/include -I${VULKAN_SDK}/include -I$(RUNTIME_DIR)/include
DEFINES=-DGLFW_INCLUDE_NONE -DGLFW_INCLUDE_VULKAN -DLOG_PATH=\"./\" -DRESOURCE_PATH=\"./\" -DSHADER_PATH=\"./\"

LDFLAGS=-Wl,-rpath,'$$ORIGIN' -L$(BIN_DIR)
LDLIBS=-lRuntime

.PHONY: $(TARGET)
$(TARGET): $(OUT)
	@echo Built $(TARGET)

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%$(SRC_EXT)
	@echo Compiling $*$(SRC_EXT)...
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) -MMD -MP -c $< -o $@
