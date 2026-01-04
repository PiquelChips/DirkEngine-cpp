# COMPILES SHADERS

## INT_DIR -- the intermediate folder to use
## SHADER_DIR -- the shader's source code

SHADER_SRC=$(SHADER_DIR)/Source
SHADERS=$(shell find $(SHADER_SRC) -type f)
SPV=$(SHADERS:$(SHADER_SRC)/%=$(INT_DIR)/%.spv)

.PHONY: all
all: $(SPV)

$(INT_DIR)/%.spv: $(SHADER_SRC)/%
	@echo Compiling shader $*...
	@mkdir -p $(dir $@)
	@glslc $< -o $@
