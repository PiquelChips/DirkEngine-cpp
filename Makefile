BIN_DIR=Engine/Binaries
SCRIPTS_DIR=Engine/Scripts
BUILD_TOOL=$(BIN_DIR)/DirkBuildTool
BUILD_TOOL_DIR=Engine/Programs/DirkBuildTool
BUILD_TOOL_SRC=$(shell find $(BUILD_TOOL_DIR)/Source -type f -name '*')

EDITOR=$(BIN_DIR)/DirkEditor

.PHONY: clean run build setup
run: build
	@$(EDITOR)

setup:
	@sh $(SCRIPTS_DIR)/Setup.sh

build: $(BUILD_TOOL)
	@$(BUILD_TOOL)

clean: $(BUILD_TOOL)
	@$(BUILD_TOOL) clean

$(BUILD_TOOL): $(BUILD_TOOL_SRC)
	@sh $(SCRIPTS_DIR)/DirkBuildTool.sh
