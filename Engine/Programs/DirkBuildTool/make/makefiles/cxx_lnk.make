# LINKS WITH CXX

LDFLAGS+= -L$(BIN_DIR)

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
