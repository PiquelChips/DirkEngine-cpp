BUILD_TOOL=Binaries/DirkBuildTool
BUILD_TOOL_DIR=Engine/Build/DirkBuildTool
BUILD_TOOL_SRC=$(shell find $(BUILD_TOOL_DIR) -type f -name '*.go')

EDITOR=Binaries/DirkEditor

.PHONY: clean setup projectfiles clangdb build run
run: build
	@$(EDITOR)

build: $(BUILD_TOOL)
	@$(BUILD_TOOL) build

setup: $(BUILD_TOOL)
	@$(BUILD_TOOL) setup

projectfiles: $(BUILD_TOOL)
	@$(BUILD_TOOL) projectfiles

clangdb: $(BUILD_TOOL)
	@$(BUILD_TOOL) clangdb

clean:
	@rm -rf Intermediate Saved Binaries compile_commands.json

$(BUILD_TOOL): $(BUILD_TOOL_SRC)
	@echo Building build tool...
	@go build -o $@ $(BUILD_TOOL_DIR)/main.go
