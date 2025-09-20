# COMPILES OBJECT FILES

## OBJ_DIR -- output dir of object files
## SRC_DIR -- the dir with all the source files
## SRC_EXT -- the extension of source code files
## CFLAGS -- compilation flags
## CXXFLAGS -- compilation flags
## DEFINES -- preprocessor defines

CXXFLAGS+= -Iinclude

$(OBJ_DIR)/%.o: $(SRC_DIR)/%$(SRC_EXT)
	@echo Compiling $*$(SRC_EXT)...
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) -MMD -MP -c $< -o $@
