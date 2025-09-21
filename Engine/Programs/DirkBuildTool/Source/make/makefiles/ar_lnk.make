# ARCHIVES OBJECT FILES
# should always come after base.make

$(OUT): $(OBJ)
	@echo Linking $(TARGET)...
	@mkdir -p $(dir $@)
	@$(AR) -rcs "$@" $^
