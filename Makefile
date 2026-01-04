BIN_DIR=Engine/Binaries
BUILD_TOOL=$(BIN_DIR)/DirkBuildTool
BUILD_TOOL_DIR=Engine/Programs/DirkBuildTool
BUILD_TOOL_SRC=$(shell find $(BUILD_TOOL_DIR)/Source -type f -name '*')

EDITOR=$(BIN_DIR)/Editor

.PHONY: clean run build
run: build
	@$(EDITOR)

build: $(BUILD_TOOL)
	@$(BUILD_TOOL)

clean: $(BUILD_TOOL)
	@$(BUILD_TOOL) clean

$(BUILD_TOOL): $(BUILD_TOOL_SRC)
	@echo Building build tool...
	@go build -C $(BUILD_TOOL_DIR)/Source -o ../../../../$@ main.go
