# COMPILES SHADERS

## ROOT_DIR -- engine root dir
## SHADER_DIR -- the shader's source code

INT_DIR=$(ROOT_DIR)/Intermediate/Shaders

SHADERS=$(shell find $(SHADER_DIR)/Source -type f)
SPV=$(SHADERS:$(SHADERS_DIR)/%=$(SHADERS_OUT_DIR)/%.spv)

$(INT_DIR)/%.spv: $(SHADER_DIR)/Source/%
	@echo Compiling shader $*...
	@mkdir -p $(dir $@)
	@glslc $< -o $@
