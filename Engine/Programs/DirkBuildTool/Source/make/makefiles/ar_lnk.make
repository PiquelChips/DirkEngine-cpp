# ARCHIVES OBJECT FILES

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(AR) -rcs "$@" $^
