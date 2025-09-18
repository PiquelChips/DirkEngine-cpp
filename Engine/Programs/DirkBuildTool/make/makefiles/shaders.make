# COMPILES SHADERS

## ROOT_DIR -- engine root dir

INT_DIR=$(ROOT_DIR)/Intermediate/Shaders
SHADERS_DIR=$(ROOT_DIR)/Engine/Shaders/src

SHADERS_OUT_DIR=$(INT_DIR)/shaders
SHADERS=$(shell find $(SHADERS_DIR) -type f)
SPV=$(SHADERS:$(SHADERS_DIR)/%=$(SHADERS_OUT_DIR)/%.spv)

$(SHADERS_OUT_DIR)/%.spv: $(SHADERS_DIR)/%
	@echo Compiling shader $*...
	@mkdir -p $(dir $@)
	@glslc $< -o $@
