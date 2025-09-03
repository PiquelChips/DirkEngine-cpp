TARGET=Editor
DEPS=Runtime
SOURCE_DIR=../Source
BIN_DIR=../../Binaries

.PHONY: run $(TARGET) $(DEPS)
run: $(TARGET)
	@$(MAKE) -C $(SOURCE_DIR)/$(TARGET) run

$(TARGET): $(DEPS)
	@$(MAKE) -C $(SOURCE_DIR)/$@ $@

$(DEPS):
	@$(MAKE) -C $(SOURCE_DIR)/$@ $@
