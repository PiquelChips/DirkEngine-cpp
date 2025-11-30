BUILD_TOOL=Binaries/DirkBuildTool
BUILD_TOOL_DIR=Engine/Programs/DirkBuildTool
BUILD_TOOL_SRC=$(shell find $(BUILD_TOOL_DIR)/Source -type f -name '*')

EDITOR=Binaries/DirkEditor

.PHONY: clean run build
run: build
	@$(EDITOR)

build: $(BUILD_TOOL)
	@$(BUILD_TOOL)

clean: $(BUILD_TOOL)
	@$(BUILD_TOOL) clean

clean-all: $(BUILD_TOOL)
	@$(BUILD_TOOL) clean-all

clean-setup: $(BUILD_TOOL)
	@$(BUILD_TOOL) clean-setup

$(BUILD_TOOL): $(BUILD_TOOL_SRC)
	@echo Building build tool...
	@go build -C $(BUILD_TOOL_DIR)/Source -o ../../../../$@ main.go
