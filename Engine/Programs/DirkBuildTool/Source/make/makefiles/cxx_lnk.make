# LINKS WITH CXX
# should always come after base.make

LDFLAGS+= -L$(BIN_DIR)

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
